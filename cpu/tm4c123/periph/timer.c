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

#define TIMER_NUMOF 4

#define TIMER_0_EN  1
#define TIMER_1_EN  1
#define TIMER_2_EN  1
#define TIMER_3_EN  1

#include "cpu.h"
#include "board.h"
#include "sched.h"
#include "thread.h"
#include "periph_conf.h"
#include "periph/timer.h"

#include "board.h"

#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"

#define ENABLE_DEBUG 1
#include "debug.h"

#define TIMER_0_DEV TIMER0
#define TIMER_1_DEV TIMER1
#define TIMER_2_DEV TIMER2
#define TIMER_3_DEV TIMER3

#define TIMER_0_ISR isr_tim0a
#define TIMER_1_ISR isr_tim1a
#define TIMER_2_ISR isr_tim2a
#define TIMER_3_ISR isr_tim3a

/** Unified IRQ handler for all timers */
static inline void irq_handler(tim_t timer, TIMER0_Type *dev);

/** Type for timer state */
typedef struct {
    void (*cb)(int);
} timer_conf_t;

/** Timer state memory */
timer_conf_t config[TIMER_NUMOF];


/**
 * @brief Initialize the given timer
 *
 * Each timer device is running with the given speed. Each can contain one or more channels
 * as defined in periph_conf.h. The timer is configured in up-counting mode and will count
 * until TIMER_x_MAX_VALUE as defined in used board's periph_conf.h until overflowing.
 *
 * The timer will be started automatically after initialization with interrupts enabled.
 *
 * @param[in] dev           the timer to initialize
 * @param[in] ticks_per_us  the timers speed in ticks per us
 * @param[in] callback      this callback is called in interrupt context, the emitting channel is
 *                          passed as argument
 *
 * @return                  returns 0 on success, -1 if speed not applicable of unknown device given
 */
int timer_init(tim_t dev, unsigned int ticks_per_us, void (*callback)(int)) {
    DEBUG("timer_init()\n");
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);

    ROM_IntMasterEnable();

    return 0;
}

/**
 * @brief Set a given timer channel for the given timer device. The callback given during
 * initialization is called when timeout ticks have passed after calling this function
 *
 * @param[in] dev           the timer device to set
 * @param[in] channel       the channel to set
 * @param[in] timeout       timeout in ticks after that the registered callback is executed
 *
 * @return                  1 on success, -1 on error
 */
int timer_set(tim_t dev, int channel, unsigned int timeout) {
    // TODO

    return timer_set_absolute(dev, channel, timeout);
}

/**
 * @brief Set an absolute timeout value for the given channel of the given timer device
 *
 * @param[in] dev           the timer device to set
 * @param[in] channel       the channel to set
 * @param[in] value         the absolute compare value when the callback will be triggered
 *
 * @return                  1 on success, -1 on error
 */
int timer_set_absolute(tim_t dev, int channel, unsigned int value) {
    DEBUG("timer_set_absolute(%d, %d (%d))\n", channel, value, ROM_SysCtlClockGet());
    int timer;
    IRQn_Type irq;

    switch(channel) {
        case 0:
            value = ROM_SysCtlClockGet() * 10;
            timer = TIMER0_BASE;
            irq = TIMER0A_IRQn;
            break;
        case 1:
            value = ROM_SysCtlClockGet() * 5;
            timer = TIMER1_BASE;
            irq = TIMER1A_IRQn;
            break;
        case 2:
            timer = TIMER2_BASE;
            irq = TIMER2A_IRQn;
            return -1; // definies may be wrong?
            break;
        case 3:
            timer = TIMER3_BASE;
            irq = TIMER3A_IRQn;
            return -1;
            break;
        default:
            return -1;
    }

    printf("IRQn_Type = %d\n", irq);

    ROM_TimerConfigure(timer, TIMER_CFG_PERIODIC);
    ROM_TimerLoadSet(timer, TIMER_A, value);

    //
    // Setup the interrupts for the timer timeouts.
    //
    NVIC_EnableIRQ(irq);
    ROM_TimerIntEnable(timer, TIMER_TIMA_TIMEOUT);

    //
    // Enable the timers.
    //
    ROM_TimerEnable(timer, TIMER_A);

    return 1;
}

/**
 * @brief Clear the given channel of the given timer device
 *
 * @param[in] dev           the timer device to clear
 * @param[in] channel       the channel on the given device to clear
 *
 * @return                  1 on success, -1 on error
 */
int timer_clear(tim_t dev, int channel) {
    DEBUGF("todo\n");

    return 1;
}

/**
 * @brief Read the current value of the given timer device
 *
 * @param[in] dev           the timer to read the current value from
 *
 * @return                  the timers current value
 */
unsigned int timer_read(tim_t dev) {
    DEBUGF("todo\n");

    return 0;
}

/**
 * @brief Start the given timer. This function is only needed if the timer was stopped manually before
 *
 * @param[in] dev           the timer device to stop
 */
void timer_start(tim_t dev) {
    DEBUGF("todo\n");
}

/**
 * @brief Stop the given timer - this will effect all of the timer's channels
 *
 * @param[in] dev           the timer to stop
 */
void timer_stop(tim_t dev) {
    DEBUGF("todo\n");
}

/**
 * @brief Enable the interrupts for the given timer
 *
 * @param[in] dev           timer to enable interrupts for
 */
void timer_irq_enable(tim_t dev) {
    DEBUGF("todo\n");
}

/**
 * @brief Disable interrupts for the given timer
 *
 * @param[in] dev           the timer to disable interrupts for
 */
void timer_irq_disable(tim_t dev) {
    DEBUGF("todo\n");
}

/**
 * @brief Reset the up-counting value to zero for the given timer
 *
 * Note that this function effects all currently set channels and it can lead to non-deterministic timeouts
 * if any channel is active when this function is called.
 *
 * @param[in] dev           the timer to reset
 */
void timer_reset(tim_t dev) {
    DEBUGF("todo\n");
}

#if TIMER_0_EN
__attribute__ ((naked)) void TIMER_0_ISR(void)
{
    ISR_ENTER();
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    RED_LED_ON;
    irq_handler(TIMER_0, TIMER_0_DEV);
    ISR_EXIT();
}
#endif
#if TIMER_1_EN
__attribute__ ((naked)) void TIMER_1_ISR(void)
{
    ISR_ENTER();
    ROM_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    GREEN_LED_ON;
    irq_handler(TIMER_1, TIMER_1_DEV);
    ISR_EXIT();
}
#endif
#if TIMER_2_EN
__attribute__ ((naked)) void TIMER_2_ISR(void)
{
    ISR_ENTER();
    ROM_TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);

    BLUE_LED_ON;
    irq_handler(TIMER_2, TIMER_2_DEV);
    ISR_EXIT();
}
#endif
#if TIMER_3_EN
__attribute__ ((naked)) void TIMER_3_ISR(void)
{
    ISR_ENTER();
    ROM_TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);

    RED_LED_ON;
    irq_handler(TIMER_3, TIMER_3_DEV);
    ISR_EXIT();
}
#endif

static inline void irq_handler(tim_t timer, TIMER0_Type *dev)
{
    ROM_TimerIntClear((uint32_t) dev, TIMER_TIMA_TIMEOUT);
    RED_LED_ON;

    if (dev == TIMER_0_DEV) {
        config[timer].cb(0);
    } else if (dev == TIMER_1_DEV) {
        config[timer].cb(1);
    } else if (dev == TIMER_2_DEV) {
        config[timer].cb(2);
    } else if (dev == TIMER_3_DEV) {
        config[timer].cb(3);
    }

    if (sched_context_switch_request) {
        thread_yield();
    }
}