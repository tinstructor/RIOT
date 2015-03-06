/*
 * Copyright (C) 2015 volatiles UG
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @addtogroup  driver_periph
 * @{
 *
 * @file
 * @brief       Low-level GPIO driver implementation
 *
 * @author      Benjamin Valentin <benjamin.valentin@volatiles.de>
 *
 * @}
 */

#include "periph/gpio.h"
#include "periph_conf.h"
#include "cpu.h"
#include "driverlib/gpio.h"
#include "driverlib/hw_types.h"
#include "driverlib/hw_gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"

#include <stdint.h>

/* guard file in case no GPIO devices are defined */
#if GPIO_NUMOF

typedef struct {
    gpio_cb_t cb;
    void *arg;
} gpio_state_t;

static gpio_state_t gpio_config[GPIO_NUMOF];

/* static port mappings */
static const uint32_t gpio_port_map[GPIO_NUMOF] = {
#if GPIO_0_EN
    [GPIO_0] = GPIO_0_PORT,
#endif
#if GPIO_1_EN
    [GPIO_1] = GPIO_1_PORT,
#endif
#if GPIO_2_EN
    [GPIO_2] = GPIO_2_PORT,
#endif
#if GPIO_3_EN
    [GPIO_3] = GPIO_3_PORT,
#endif
#if GPIO_4_EN
    [GPIO_4] = GPIO_4_PORT,
#endif
#if GPIO_5_EN
    [GPIO_5] = GPIO_5_PORT,
#endif
#if GPIO_6_EN
    [GPIO_6] = GPIO_6_PORT,
#endif
#if GPIO_7_EN
    [GPIO_7] = GPIO_7_PORT,
#endif
#if GPIO_8_EN
    [GPIO_8] = GPIO_8_PORT,
#endif
#if GPIO_9_EN
    [GPIO_9] = GPIO_9_PORT,
#endif
#if GPIO_10_EN
    [GPIO_10] = GPIO_10_PORT,
#endif
#if GPIO_11_EN
    [GPIO_11] = GPIO_11_PORT,
#endif
#if GPIO_12_EN
    [GPIO_12] = GPIO_12_PORT,
#endif
#if GPIO_13_EN
    [GPIO_13] = GPIO_13_PORT,
#endif
#if GPIO_14_EN
    [GPIO_14] = GPIO_14_PORT,
#endif
#if GPIO_15_EN
    [GPIO_15] = GPIO_15_PORT,
#endif
};

/* static port mappings */
static const uint32_t gpio_sysctl_map[GPIO_NUMOF] = {
#if GPIO_0_EN
    [GPIO_0] = GPIO_0_SYSCTL,
#endif
#if GPIO_1_EN
    [GPIO_1] = GPIO_1_SYSCTL,
#endif
#if GPIO_2_EN
    [GPIO_2] = GPIO_2_SYSCTL,
#endif
#if GPIO_3_EN
    [GPIO_3] = GPIO_3_SYSCTL,
#endif
#if GPIO_4_EN
    [GPIO_4] = GPIO_4_SYSCTL,
#endif
#if GPIO_5_EN
    [GPIO_5] = GPIO_5_SYSCTL,
#endif
#if GPIO_6_EN
    [GPIO_6] = GPIO_6_SYSCTL,
#endif
#if GPIO_7_EN
    [GPIO_7] = GPIO_7_SYSCTL,
#endif
#if GPIO_8_EN
    [GPIO_8] = GPIO_8_SYSCTL,
#endif
#if GPIO_9_EN
    [GPIO_9] = GPIO_9_SYSCTL,
#endif
#if GPIO_10_EN
    [GPIO_10] = GPIO_10_SYSCTL,
#endif
#if GPIO_11_EN
    [GPIO_11] = GPIO_11_SYSCTL,
#endif
#if GPIO_12_EN
    [GPIO_12] = GPIO_12_SYSCTL,
#endif
#if GPIO_13_EN
    [GPIO_13] = GPIO_13_SYSCTL,
#endif
#if GPIO_14_EN
    [GPIO_14] = GPIO_14_SYSCTL,
#endif
#if GPIO_15_EN
    [GPIO_15] = GPIO_15_SYSCTL,
#endif
};

/* static pin mappings */
static const uint8_t gpio_pin_map[GPIO_NUMOF] = {
#if GPIO_0_EN
    [GPIO_0] = GPIO_0_PIN,
#endif
#if GPIO_1_EN
    [GPIO_1] = GPIO_1_PIN,
#endif
#if GPIO_2_EN
    [GPIO_2] = GPIO_2_PIN,
#endif
#if GPIO_3_EN
    [GPIO_3] = GPIO_3_PIN,
#endif
#if GPIO_4_EN
    [GPIO_4] = GPIO_4_PIN,
#endif
#if GPIO_5_EN
    [GPIO_5] = GPIO_5_PIN,
#endif
#if GPIO_6_EN
    [GPIO_6] = GPIO_6_PIN,
#endif
#if GPIO_7_EN
    [GPIO_7] = GPIO_7_PIN,
#endif
#if GPIO_8_EN
    [GPIO_8] = GPIO_8_PIN,
#endif
#if GPIO_9_EN
    [GPIO_9] = GPIO_9_PIN,
#endif
#if GPIO_10_EN
    [GPIO_10] = GPIO_10_PIN,
#endif
#if GPIO_11_EN
    [GPIO_11] = GPIO_11_PIN,
#endif
#if GPIO_12_EN
    [GPIO_12] = GPIO_12_PIN,
#endif
#if GPIO_13_EN
    [GPIO_13] = GPIO_13_PIN,
#endif
#if GPIO_14_EN
    [GPIO_14] = GPIO_14_PIN,
#endif
#if GPIO_15_EN
    [GPIO_15] = GPIO_15_PIN,
#endif
};

/* static irq mappings */
static const uint32_t gpio_irq_map[GPIO_NUMOF] = {
#if GPIO_0_EN
    [GPIO_0] = GPIO_0_IRQ,
#endif
#if GPIO_1_EN
    [GPIO_1] = GPIO_1_IRQ,
#endif
#if GPIO_2_EN
    [GPIO_2] = GPIO_2_IRQ,
#endif
#if GPIO_3_EN
    [GPIO_3] = GPIO_3_IRQ,
#endif
#if GPIO_4_EN
    [GPIO_4] = GPIO_4_IRQ,
#endif
#if GPIO_5_EN
    [GPIO_5] = GPIO_5_IRQ,
#endif
#if GPIO_6_EN
    [GPIO_6] = GPIO_6_IRQ,
#endif
#if GPIO_7_EN
    [GPIO_7] = GPIO_7_IRQ,
#endif
#if GPIO_8_EN
    [GPIO_8] = GPIO_8_IRQ,
#endif
#if GPIO_9_EN
    [GPIO_9] = GPIO_9_IRQ,
#endif
#if GPIO_10_EN
    [GPIO_10] = GPIO_10_IRQ,
#endif
#if GPIO_11_EN
    [GPIO_11] = GPIO_11_IRQ,
#endif
#if GPIO_12_EN
    [GPIO_12] = GPIO_12_IRQ,
#endif
#if GPIO_13_EN
    [GPIO_13] = GPIO_13_IRQ,
#endif
#if GPIO_14_EN
    [GPIO_14] = GPIO_14_IRQ,
#endif
#if GPIO_15_EN
    [GPIO_15] = GPIO_15_IRQ,
#endif
};

static void gpio_pin_unlock(uint32_t port, uint8_t pin) {
    HWREG(port + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(port + GPIO_O_CR)  |= pin;
    HWREG(port + GPIO_O_LOCK) = 0;
}

int gpio_init_out(gpio_t dev, gpio_pp_t pullup) {
	ROM_SysCtlPeripheralEnable(gpio_sysctl_map[dev]);
	gpio_pin_unlock(gpio_port_map[dev], gpio_pin_map[dev]);
	ROM_GPIOPinTypeGPIOOutput(gpio_port_map[dev], gpio_pin_map[dev]);

	return 0;
}

int gpio_init_in(gpio_t dev, gpio_pp_t pullup) {
	ROM_SysCtlPeripheralEnable(gpio_sysctl_map[dev]);
	gpio_pin_unlock(gpio_port_map[dev], gpio_pin_map[dev]);
	ROM_GPIOPinTypeGPIOInput(gpio_port_map[dev], gpio_pin_map[dev]);

	return 0;
}

int gpio_init_int(gpio_t dev, gpio_pp_t pullup, gpio_flank_t flank, gpio_cb_t cb, void *arg) {
    /* configure pin as input */
    int res = gpio_init_in(dev, pullup);
    if (res < 0) {
        return res;
    }

    // TODO
    return 0;
}

void gpio_irq_enable(gpio_t dev) {

}

void gpio_irq_disable(gpio_t dev) {

}

int gpio_read(gpio_t dev) {
	return ROM_GPIOPinRead(gpio_port_map[dev], gpio_pin_map[dev]);
}

void gpio_set(gpio_t dev) {
	ROM_GPIOPinWrite(gpio_port_map[dev], gpio_pin_map[dev], gpio_pin_map[dev]);
}

void gpio_clear(gpio_t dev) {
	ROM_GPIOPinWrite(gpio_port_map[dev], gpio_pin_map[dev], 0);
}

void gpio_toggle(gpio_t dev) {
    if (gpio_read(dev)) {
        gpio_clear(dev);
    } else {
        gpio_set(dev);
    }
}

void gpio_write(gpio_t dev, int value) {
	ROM_GPIOPinWrite(gpio_port_map[dev], gpio_pin_map[dev], value);
}

#endif