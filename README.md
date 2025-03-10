# NextFab IoT lsm303dlhc Sensor

This repository contains firmware for the NextFab IoT lsm303dlhc sensor.

## Usage

- Initialize Zephyr workspace
```bash
west init -m https://github.com/catie-aq/nextfab_zephyr-lsm303dlhc.git nextfab_lsm303dlhc
cd nextfab_lsm303dlhc
west update
```

- Compile the project
```bash
west build -b <TARGET> app/<project>
```

- Program the target device
```bash
west flash
```

## Applications
- [lsm303dlhc Sensor](app/lsm303dlhc/): lsm303dlhc sensor firmware for NextFab IoT sensors
- [Gateway](app/gateway/): Gateway UART-UDP for NextFab IoT sensors
