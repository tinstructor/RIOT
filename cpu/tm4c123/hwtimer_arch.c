/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_stm32f4
 * @{
 *
 * @file
 * @brief       Implementation of the kernels hwtimer interface
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include "arch/hwtimer_arch.h"
#include "board.h"
#include "thread.h"

#include "driverlib/timer.h"
#include "driverlib/rom.h"

/**
 * @brief Callback function that is given to the low-level timer
 *
 * @param[in] channel   the channel of the low-level timer that was triggered
 */
void irq_handler(int channel);

/**
 * @brief Hold a reference to the hwtimer callback
 */
void (*timeout_handler)(int);

void hwtimer_arch_init(void (*handler)(int), uint32_t fcpu)
{
	ROM_TimerEnable(TIMER0_BASE, TIMER_A);
}

void hwtimer_arch_enable_interrupt(void)
{
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
}

void hwtimer_arch_disable_interrupt(void)
{
    ROM_TimerIntDisable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
}

void hwtimer_arch_set(unsigned long offset, short timer)
{
//    timer_set(HW_TIMER, timer, offset);
}

void hwtimer_arch_set_absolute(unsigned long value, short timer)
{
//    timer_set_absolute(HW_TIMER, timer, value);
}

void hwtimer_arch_unset(short timer)
{
//    timer_clear(HW_TIMER, timer);
}

unsigned long hwtimer_arch_now(void)
{
//    return timer_read(HW_TIMER);
	return 0;
}

void irq_handler(int channel)
{
    // timeout_handler((short)(channel));
}
