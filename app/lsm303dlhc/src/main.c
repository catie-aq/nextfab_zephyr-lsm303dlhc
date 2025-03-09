/*
 * Copyright (c) 2025 CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <string.h>

LOG_MODULE_REGISTER(accelero, LOG_LEVEL_DBG);

#define ACCEL_READ_INTERVAL_MS 20
#define UART_TX_DEVICE         DT_NODELABEL(usart2)
#define MSGQ_MAX_ITEMS         10

static const struct device *const uart_accel = DEVICE_DT_GET(UART_TX_DEVICE);
static const struct device *const accel_device = DEVICE_DT_GET(DT_NODELABEL(lsm303agr_accel));

const struct gpio_dt_spec driver_enable =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), driver_enable_gpios);

K_THREAD_STACK_DEFINE(accel_thread_stack, 512);
K_THREAD_STACK_DEFINE(uart_thread_stack, 512);
struct k_thread accel_thread_data;
struct k_thread uart_thread_data;

struct accel_data_t {
	uint32_t timestamp;
	int32_t x, y, z;
};

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(accel_msgq, sizeof(struct accel_data_t), MSGQ_MAX_ITEMS, 4);

void accel_thread(void *arg1, void *arg2, void *arg3)
{
	struct sensor_value accel_x, accel_y, accel_z;
	struct accel_data_t data;

	while (true) {
		if (sensor_sample_fetch(accel_device) == 0) {
			sensor_channel_get(accel_device, SENSOR_CHAN_ACCEL_X, &accel_x);
			sensor_channel_get(accel_device, SENSOR_CHAN_ACCEL_Y, &accel_y);
			sensor_channel_get(accel_device, SENSOR_CHAN_ACCEL_Z, &accel_z);

			data.timestamp = k_uptime_get_32();
			data.x = accel_x.val1 * 1000 + accel_x.val2 / 1000;
			data.y = accel_y.val1 * 1000 + accel_y.val2 / 1000;
			data.z = accel_z.val1 * 1000 + accel_z.val2 / 1000;

			if (k_msgq_put(&accel_msgq, &data, K_NO_WAIT) != 0) {
				k_msgq_purge(&accel_msgq);
				k_msgq_put(&accel_msgq, &data, K_NO_WAIT);
			}
		}
		k_msleep(ACCEL_READ_INTERVAL_MS);
	}
}

void send_accel_data_uart(struct accel_data_t *data)
{
	if (!device_is_ready(uart_accel)) {
		LOG_ERR("UART TX device not ready");
		return;
	}

	char message[100];
	int message_length = snprintf(message, sizeof(message), "T=%u,X=%d,Y=%d,Z=%d\n\r",
				      data->timestamp, data->x, data->y, data->z);

	gpio_pin_set_dt(&driver_enable, 1);

	for (size_t i = 0; i < message_length; i++) {
		uart_poll_out(uart_accel, message[i]);
	}
}

void uart_thread(void *arg1, void *arg2, void *arg3)
{
	struct accel_data_t data;

	while (true) {
		k_msgq_get(&accel_msgq, &data, K_FOREVER);
		send_accel_data_uart(&data);
	}
}

int main()
{
	int ret;

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

	gpio_pin_set_dt(&driver_enable, 0);

	k_thread_create(&accel_thread_data, accel_thread_stack,
			K_THREAD_STACK_SIZEOF(accel_thread_stack), accel_thread, NULL, NULL, NULL,
			7, 0, K_NO_WAIT);

	k_thread_create(&uart_thread_data, uart_thread_stack,
			K_THREAD_STACK_SIZEOF(uart_thread_stack), uart_thread, NULL, NULL, NULL, 7,
			0, K_NO_WAIT);

	k_sleep(K_FOREVER);

	return 0;
}
