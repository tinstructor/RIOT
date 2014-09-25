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
 * @brief       Peripheral MCU configuration for the STM32F3discovery board
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef __PERIPH_CONF_H
#define __PERIPH_CONF_H

#define UART_IRQ_PRIO       1
#define UART_NUMOF          (2U)
#define UART_0_EN           1
#define UART_0_ISR          isr_uart0

#define UART_1_EN           1
#define UART_1_ISR          isr_uart1

#define UART_0_IRQ_CHAN     UART0_IRQn
#define UART_1_IRQ_CHAN     UART1_IRQn

#define UART_0_DEV          UART0
#define UART_1_DEV          UART1

#endif /* __PERIPH_CONF_H */
