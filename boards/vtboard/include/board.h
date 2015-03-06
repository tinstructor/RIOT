/*
 * Copyright (C) 2014 volatiles UG (haftungsbeschränkt)
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @defgroup    board_ek-tm4c123gxl
 * @ingroup     boards
 * @brief       Board specific files for the EK-TM4C123GXL Launchpad evaluation board
 * @{
 *
 * @file
 * @brief       Board specific definitions for the EK-TM4C123GXL Launchpad evaluation board
 *
 * @author      Benjamin Valentin <benjamin.valentin@volatiles.de>
 */

#ifndef __BOARD_H
#define __BOARD_H

#include "cpu.h"
#include "periph_conf.h"

/**
 * Define the nominal CPU core clock in this board
 */
#define F_CPU               (80000000UL)

/**
 * @name Assign the hardware timer
 */
#define HW_TIMER            TIMER_0

/**
 * @name Define the UART used for stdio
 */
#define STDIO               UART_0
#define STDIO_BAUDRATE      (115200U)
#define STDIO_RX_BUFSIZE    (64U)

#define _LED_RED            (*((volatile unsigned int *)(GPIOF_BASE + ((0x2 << 2)))))
#define _LED_BLUE           (*((volatile unsigned int *)(GPIOF_BASE + ((0x4 << 2)))))
#define _LED_GREEN          (*((volatile unsigned int *)(GPIOF_BASE + ((0x8 << 2)))))

#define LED_RED_ON          (_LED_RED = 0x2)
#define LED_RED_OFF         (_LED_RED = 0x0)
#define LED_RED_TOGGLE      (_LED_RED^= 0x2)

#define LED_BLUE_ON         (_LED_BLUE = 0x4)
#define LED_BLUE_OFF        (_LED_BLUE = 0x0)
#define LED_BLUE_TOGGLE     (_LED_BLUE^= 0x4)

#define LED_GREEN_ON        (_LED_GREEN = 0x8)
#define LED_GREEN_OFF       (_LED_GREEN = 0x0)
#define LED_GREEN_TOGGLE    (_LED_GREEN^= 0x8)

/**
 * @brief Initialize board specific hardware, including clock, LEDs and std-IO
 */
void board_init(void);

#endif /** __BOARD_H */
/** @} */
