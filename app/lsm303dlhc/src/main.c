#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* GPIO configuration */
const struct gpio_dt_spec driver_enable =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), driver_enable_gpios);



void main(void) {

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

    printk("Hello, world\n");
}