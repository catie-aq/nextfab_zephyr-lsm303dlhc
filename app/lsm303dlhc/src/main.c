/*
 * Copyright (c) 2025 CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(accelero, LOG_LEVEL_DBG);

#define ACCEL_READ_INTERVAL_MS 20
#define UART_TX_DEVICE         DT_NODELABEL(usart2)

static const struct device *const uart_accel = DEVICE_DT_GET(UART_TX_DEVICE);

const struct gpio_dt_spec driver_enable =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), driver_enable_gpios);

static const struct device *const accel_device = DEVICE_DT_GET(DT_NODELABEL(lsm303agr_accel));

void send_accel_data(int32_t x, int32_t y, int32_t z)
{
	if (!device_is_ready(uart_accel)) {
		LOG_ERR("UART TX device not ready");
		return;
	}

	char message[64];
	snprintf(message, sizeof(message), "X=%d,Y=%d,Z=%d\n\r", x, y, z);

	gpio_pin_set_dt(&driver_enable, 1);
	// k_sleep(K_MSEC(10));

	for (size_t i = 0; i < strlen(message); i++) {
		uart_poll_out(uart_accel, message[i]);
	}

	k_sleep(K_MSEC(1));

	gpio_pin_set_dt(&driver_enable, 0);
}

int main()
{
	int ret;
	struct sensor_value accel_x, accel_y, accel_z;
	int32_t x, y, z;

	k_msleep(3000);

	device_init(accel_device);
	if (!device_is_ready(accel_device)) {
		return 0;
	}

	struct uart_config uart_cfg = {.baudrate = 115200,
				       .parity = UART_CFG_PARITY_NONE,
				       .stop_bits = UART_CFG_STOP_BITS_1,
				       .data_bits = UART_CFG_DATA_BITS_8,
				       .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

	if (!device_is_ready(driver_enable.port)) {
		LOG_ERR("GPIO device not ready");
		return 0;
	}

	ret = gpio_pin_configure_dt(&driver_enable, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("GPIO config failed: %d", ret);
		return 0;
	}

	gpio_pin_set_dt(&driver_enable, 0);

	ret = uart_configure(uart_accel, &uart_cfg);
	if (ret < 0) {
		LOG_ERR("UART config failed: %d", ret);
		return 0;
	}

	if (!device_is_ready(accel_device)) {
		LOG_ERR("Accelerometer device not ready");
		return 0;
	}

	while (true) {
		if (sensor_sample_fetch(accel_device) == 0) {
			sensor_channel_get(accel_device, SENSOR_CHAN_ACCEL_X, &accel_x);
			sensor_channel_get(accel_device, SENSOR_CHAN_ACCEL_Y, &accel_y);
			sensor_channel_get(accel_device, SENSOR_CHAN_ACCEL_Z, &accel_z);

			x = accel_x.val1 * 1000 + accel_x.val2 / 1000;
			y = accel_y.val1 * 1000 + accel_y.val2 / 1000;
			z = accel_z.val1 * 1000 + accel_z.val2 / 1000;

			send_accel_data(x, y, z);
		}
		k_msleep(ACCEL_READ_INTERVAL_MS);
	}
	return 0;
}
