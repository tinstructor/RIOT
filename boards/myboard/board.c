/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     board_stm32f3discovery
 * @{
 *
 * @file
 * @brief       Board specific implementations for the STM32F3Discovery evaluation board
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include "board.h"

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"

void board_init(void)
{
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    /* set pendSV interrupt to lowest possible priority */
    NVIC_SetPriority(PendSV_IRQn, 0xff);

    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Enable the GPIO pins for the LED (PF1, PF2 & PF3).
    //
    ROM_GPIOPinTypeGPIOOutput(GPIOF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
}
