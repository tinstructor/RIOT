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

#define TIMER_0_DEV TIMER0
#define TIMER_1_DEV TIMER1
#define TIMER_2_DEV TIMER2
#define TIMER_3_DEV TIMER3
#define TIMER_4_DEV TIMER4
#define TIMER_5_DEV TIMER5

#define TIMER_0_ISR isr_tim0a
#define TIMER_1_ISR isr_tim1a
#define TIMER_2_ISR isr_tim2a
#define TIMER_3_ISR isr_tim3a
#define TIMER_4_ISR isr_tim4a
#define TIMER_5_ISR isr_tim5a

#endif /* __PERIPH_CONF_H */
