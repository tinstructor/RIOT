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
 * @brief       Low-level timer driver implementation
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdlib.h>

#define TIMER_NUMOF 2
#define TIMER_0_EN 1
#define TIMER_1_EN 1

#include "cpu.h"
#include "board.h"
#include "sched.h"
#include "thread.h"
#include "periph_conf.h"
#include "periph/timer.h"

#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"

#define TIMER_0_DEV TIMER0
#define TIMER_1_DEV TIMER1

#define TIMER_0_ISR isr_tim0a
#define TIMER_1_ISR isr_tim1a

#define TIMER_0_IRQ_CHAN TIMER0A_IRQn
#define TIMER_1_IRQ_CHAN TIMER1A_IRQn

#define TIMER_IRQ_PRIO 1


/** Unified IRQ handler for all timers */
static inline void irq_handler(tim_t timer, TIMER0_Type *dev);

/** Type for timer state */
typedef struct {
    void (*cb)(int);
} timer_conf_t;

/** Timer state memory */
timer_conf_t config[TIMER_NUMOF];


int timer_init(tim_t dev, unsigned int ticks_per_us, void (*callback)(int))
{
    TIMER0_Type *timer;

    switch (dev) {
#if TIMER_0_EN
        case TIMER_0:
            /* enable timer peripheral clock */
            ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
            /* set timer's IRQ priority */
            NVIC_SetPriority(TIMER_0_IRQ_CHAN, TIMER_IRQ_PRIO);
            /* select timer */
            timer = TIMER_0_DEV;
//            timer->PSC = TIMER_0_PRESCALER * ticks_per_us;
            break;
#endif
#if TIMER_1_EN
        case TIMER_1:
            /* enable timer peripheral clock */
            ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
            /* set timer's IRQ priority */
            NVIC_SetPriority(TIMER_1_IRQ_CHAN, TIMER_IRQ_PRIO);
            /* select timer */
            timer = TIMER_1_DEV;
//            timer->PSC = TIMER_0_PRESCALER * ticks_per_us;
            break;
#endif
        case TIMER_UNDEFINED:
        default:
            return -1;
    }

    /* set callback function */
    config[dev].cb = callback;

    /* set timer to run in counter mode */
//    timer->CR1 = 0;
//    timer->CR2 = 0;

    /* set auto-reload and prescaler values and load new values */
//    timer->EGR |= TIM_EGR_UG;

    /* enable the timer's interrupt */
    timer_irq_enable(dev);

    /* start the timer */
    timer_start(dev);

    return 0;
}

int timer_set(tim_t dev, int channel, unsigned int timeout)
{
    int now = timer_read(dev);
    return timer_set_absolute(dev, channel, now + timeout - 1);
}

int timer_set_absolute(tim_t dev, int channel, unsigned int value)
{
    TIMER0_Type *timer;

    switch (dev) {
#if TIMER_0_EN
        case TIMER_0:
            timer = TIMER_0_DEV;
            break;
#endif
#if TIMER_1_EN
        case TIMER_1:
            timer = TIMER_1_DEV;
            break;
#endif
        case TIMER_UNDEFINED:
        default:
            return -1;
    }

    ROM_TimerConfigure((uint32_t) timer, TIMER_CFG_ONE_SHOT);
    switch (channel) {
        case 0:
            ROM_TimerLoadSet((uint32_t) timer, TIMER_A, value);
            break;
        case 1:
            ROM_TimerLoadSet((uint32_t) timer, TIMER_B, value);
            break;
        default:
            return -1;
    }

    return 0;
}

int timer_clear(tim_t dev, int channel)
{
    TIMER0_Type *timer;

    switch (dev) {
#if TIMER_0_EN
        case TIMER_0:
            timer = TIMER_0_DEV;
            break;
#endif
#if TIMER_1_EN
        case TIMER_1:
            timer = TIMER_1_DEV;
            break;
#endif
        case TIMER_UNDEFINED:
        default:
            return -1;
    }

    switch (channel) {
        case 0:
            ROM_TimerIntClear((uint32_t) timer, TIMER_TIMA_TIMEOUT);
            break;
        case 1:
            ROM_TimerIntClear((uint32_t) timer, TIMER_TIMB_TIMEOUT);
            break;
        default:
            return -1;
    }

    return 0;
}

unsigned int timer_read(tim_t dev)
{
    switch (dev) {
#if TIMER_0_EN
        case TIMER_0:
            return TIMER_0_DEV->TAV;
            break;
#endif
#if TIMER_1_EN
        case TIMER_1:
            return TIMER_1_DEV->TAV;
            break;
#endif
        case TIMER_UNDEFINED:
        default:
            return 0;
    }
}

void timer_start(tim_t dev)
{
    switch (dev) {
#if TIMER_0_EN
        case TIMER_0:
            ROM_TimerEnable(TIMER0_BASE, TIMER_A);
            break;
#endif
#if TIMER_1_EN
        case TIMER_1:
            ROM_TimerEnable(TIMER1_BASE, TIMER_A);
            break;
#endif
        case TIMER_UNDEFINED:
            break;
    }
}

void timer_stop(tim_t dev)
{
    switch (dev) {
#if TIMER_0_EN
        case TIMER_0:
            ROM_TimerDisable(TIMER0_BASE, TIMER_A);
            break;
#endif
#if TIMER_1_EN
        case TIMER_1:
            ROM_TimerDisable(TIMER1_BASE, TIMER_A);
            break;
#endif
        case TIMER_UNDEFINED:
            break;
    }
}

void timer_irq_enable(tim_t dev)
{
    switch (dev) {
#if TIMER_0_EN
        case TIMER_0:
            NVIC_EnableIRQ(TIMER_0_IRQ_CHAN);
            break;
#endif
#if TIMER_1_EN
        case TIMER_1:
            NVIC_EnableIRQ(TIMER_1_IRQ_CHAN);
            break;
#endif
        case TIMER_UNDEFINED:
            break;
    }
}

void timer_irq_disable(tim_t dev)
{
    switch (dev) {
#if TIMER_0_EN
        case TIMER_0:
            NVIC_DisableIRQ(TIMER_0_IRQ_CHAN);
            break;
#endif
#if TIMER_1_EN
        case TIMER_1:
            NVIC_DisableIRQ(TIMER_1_IRQ_CHAN);
            break;
#endif
        case TIMER_UNDEFINED:
            break;
    }
}

void timer_reset(tim_t dev)
{
    switch (dev) {
#if TIMER_0_EN
        case TIMER_0:
            TIMER_0_DEV->TAV = 0;
            // what about TBV? (Value B)
            break;
#endif
#if TIMER_1_EN
        case TIMER_1:
            TIMER_1_DEV->TAV = 0;
            break;
#endif
        case TIMER_UNDEFINED:
            break;
    }
}

__attribute__ ((naked)) void TIMER_0_ISR(void)
{
    ISR_ENTER();
    irq_handler(TIMER_0, TIMER_0_DEV);
    ISR_EXIT();
}

__attribute__ ((naked)) void TIMER_1_ISR(void)
{
    ISR_ENTER();
    irq_handler(TIMER_1, TIMER_1_DEV);
    ISR_EXIT();
}

static inline void irq_handler(tim_t timer, TIMER0_Type *dev)
{
    ROM_TimerIntClear((uint32_t) dev, TIMER_TIMA_TIMEOUT);

    if (dev == TIMER_0_DEV) {
        config[timer].cb(0);
    }
    else if (dev == TIMER_1_DEV) {
        config[timer].cb(1);
    }

    if (sched_context_switch_request) {
        thread_yield();
    }
}
