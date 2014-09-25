/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @defgroup    board_stm32f3discovery STM32F3Discovery
 * @ingroup     boards
 * @brief       Board specific files for the STM32F3Discovery board
 * @{
 *
 * @file
 * @brief       Board specific definitions for the STM32F3Discovery evaluation board
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef __BOARD_H
#define __BOARD_H

#include "cpu.h"

/**
 * Define the nominal CPU core clock in this board
 */
#define F_CPU               (50000000UL)

/**
 * @name Assign the hardware timer
 */
#define HW_TIMER            TIMER_0

#define TIMER_NUMOF         6

#define TIMER_0_EN          1
#define TIMER_1_EN          1
#define TIMER_2_EN          1
#define TIMER_3_EN          1
#define TIMER_4_EN          1
#define TIMER_5_EN          1

/**
 * @name Define the UART used for stdio
 */
#define STDIO               UART_0
#define STDIO_BAUDRATE      (115200U)
#define STDIO_BUFSIZE       (64U)

#define RED_LED_ON          (*((volatile unsigned int *)(GPIOF_BASE + ((0x2 << 2)))) = 0x2)
#define RED_LED_OFF         (*((volatile unsigned int *)(GPIOF_BASE + ((0x2 << 2)))) = 0x0)
#define BLUE_LED_ON         (*((volatile unsigned int *)(GPIOF_BASE + ((0x4 << 2)))) = 0x4)
#define BLUE_LED_OFF        (*((volatile unsigned int *)(GPIOF_BASE + ((0x4 << 2)))) = 0x0)
#define GREEN_LED_ON        (*((volatile unsigned int *)(GPIOF_BASE + ((0x8 << 2)))) = 0x8)
#define GREEN_LED_OFF       (*((volatile unsigned int *)(GPIOF_BASE + ((0x8 << 2)))) = 0x0)

/**
 * @brief Initialize board specific hardware, including clock, LEDs and std-IO
 */
void board_init(void);

#endif /** __BOARD_H */
/** @} */
