import socket
import os
import csv

UDP_IP = ""
UDP_PORT = 1502

udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_socket.bind((UDP_IP, UDP_PORT))

print(f"Écoute des paquets UDP sur {UDP_IP}:{UDP_PORT}...")

CSV_FILE = "accelero_data.csv"

file_exists = os.path.exists(CSV_FILE)

with open(CSV_FILE, mode='a', newline='') as file:
    writer = csv.writer(file)
    if not file_exists:
        writer.writerow(['Timestamp', 'X', 'Y', 'Z'])

while True:
    data, addr = udp_socket.recvfrom(512)
    message = data.decode('utf-8').strip()

    print(f"\n Message reçu de {addr}: {message}")

    if not message:
        print("Message vide reçu !.")
        continue

    try:
        parts = message.split(',')
        if len(parts) != 4:
            print(f"Mauvais format : {message}")
            continue

        timestamp_sensor = int(parts[0].split('=')[1])
        x = int(parts[1].split('=')[1])
        y = int(parts[2].split('=')[1])
        z = int(parts[3].split('=')[1])

        with open(CSV_FILE, mode='a', newline='') as file:
            writer = csv.writer(file)
            writer.writerow([timestamp_sensor, x, y, z])

        print(f"Enregistré : T={timestamp_sensor}, X={x}, Y={y}, Z={z}")

    except Exception as e:
        print(f"Erreur parsing : {e} - Message ignoré : {message}")
