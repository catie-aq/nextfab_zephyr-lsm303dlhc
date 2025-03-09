/*
 * Copyright (c) 2025 CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>

#include <string.h>

LOG_MODULE_REGISTER(gateway, LOG_LEVEL_DBG);

#define UDP_PORT       1502
#define UDP_ADDR       "192.168.1.1"
#define MSGQ_MAX_ITEMS 10
#define MSG_SIZE       64

static const struct device *const uart_gat = DEVICE_DT_GET(DT_NODELABEL(sixtron_connector_2_uart));
const struct gpio_dt_spec re_enable = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), re_gpios);
static int udp_socket;
static struct sockaddr_in udp_addr;

K_THREAD_STACK_DEFINE(udp_thread_stack, 1024);
struct k_thread udp_thread_data;

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, MSGQ_MAX_ITEMS, 4);

void udp_init(void)
{
	udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(UDP_PORT);
	inet_pton(AF_INET, UDP_ADDR, &udp_addr.sin_addr);
}

void udp_thread(void *arg1, void *arg2, void *arg3)
{
	char message[MSG_SIZE];

	while (true) {
		k_msgq_get(&uart_msgq, message, K_FOREVER);
		sendto(udp_socket, message, strlen(message), 0, (struct sockaddr *)&udp_addr,
		       sizeof(udp_addr));
		LOG_INF("Envoyé via UDP : %s", message);
	}
}

void uart_rx_cb(const struct device *dev, void *user_data)
{
	static char rx_buffer[MSG_SIZE];
	static int rx_index = 0;
	uint8_t c;

	while (uart_fifo_read(uart_gat, &c, 1) == 1) {
		if (c == '\n') {
			rx_buffer[rx_index] = '\0';

			if (k_msgq_put(&uart_msgq, rx_buffer, K_NO_WAIT) != 0) {
				k_msgq_purge(&uart_msgq);
				k_msgq_put(&uart_msgq, rx_buffer, K_NO_WAIT);
			}
			rx_index = 0;
		} else {
			if (rx_index < sizeof(rx_buffer) - 1) {
				rx_buffer[rx_index++] = c;
			}
		}
	}
}

int main()
{
	if (!device_is_ready(uart_gat)) {
		LOG_ERR("UART not ready");
		return 0;
	}

	if (!device_is_ready(re_enable.port)) {
		LOG_ERR("⚠️ RE GPIO not ready");
		return 0;
	}

	gpio_pin_configure_dt(&re_enable, GPIO_OUTPUT_INACTIVE);
	gpio_pin_set_dt(&re_enable, 0);

	uart_irq_callback_user_data_set(uart_gat, uart_rx_cb, NULL);
	uart_irq_rx_enable(uart_gat);
	udp_init();

	k_thread_create(&udp_thread_data, udp_thread_stack, K_THREAD_STACK_SIZEOF(udp_thread_stack),
			udp_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

	k_sleep(K_FOREVER);

	return 0;
}
