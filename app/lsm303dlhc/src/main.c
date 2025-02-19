/*
 * Copyright (c) 2025, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(accel_app, LOG_LEVEL_DBG);

static const struct device *const accel_device = DEVICE_DT_GET(DT_NODELABEL(lsm303agr_accel));

const struct gpio_dt_spec driver_enable =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), driver_enable_gpios);

int main(void)
{
	int ret;

	if (!device_is_ready(driver_enable.port)) {
		LOG_ERR("Error: GPIO device %s is not ready", driver_enable.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&driver_enable, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Error %d: Failed to configure GPIO pin for driver_enable", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&driver_enable, 1);
	if (ret < 0) {
		LOG_ERR("Error %d: Failed to set GPIO pin for driver_enable", ret);
		return ret;
	}

	device_init(accel_device);
	if (!device_is_ready(accel_device)) {
		return 0;
	}
	LOG_INF("Accelerometer is ready!");

	while (1) {
		k_sleep(K_MSEC(2000));
	}
	return 0;
}
