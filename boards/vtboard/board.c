/*
 * Copyright (C) 2014 volatiles UG (haftungsbeschränkt)
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     board_ek-tm4c123gxl
 * @{
 *
 * @file
 * @brief       Board specific implementations for the EK-TM4C123GXL Launchpad evaluation board
 *
 * @author      Benjamin Valentin <benjamin.valentin@volatiles.de>
 *
 * @}
 */

#include "board.h"
#include "periph/gpio.h"

#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"

void board_init(void)
{
    cpu_init();

    /* Enable the GPIO Peripheral used by the UART */
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    /* Configure GPIO Pins for UART mode */
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIOA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	gpio_init_out(GPIO_1, 0);
	gpio_set(GPIO_1);
}
