/*
 * Copyright (C) 2014 volatiles
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_tm4c123
 * @{
 *
 * @file
 * @brief       Low-level timer driver implementation, uses full-with (32 bit) timers
 *
 * @author      Benjamin Valentin <benjamin.valentin@volatiles.de>
 *
 * @}
 */

#include <stdlib.h>

#include "board.h"
#include "sched.h"
#include "thread.h"
#include "periph_conf.h"
#include "periph/timer.h"
#include "hwtimer_cpu.h"

#include "board.h"

#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"

#define ENABLE_DEBUG 0
#include "debug.h"

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

#define TIMER_MODE TIMER_CFG_ONE_SHOT_UP

/** Unified IRQ handler for all timers */
static inline void irq_handler(tim_t timer, TIMER0_Type *dev);

/** Type for timer state */
typedef struct {
    void (*cb)(int);
} timer_conf_t;

/** Timer state memory */
timer_conf_t config[TIMER_NUMOF];

static int get_timer_base(tim_t dev) {
    switch(dev) {
        case TIMER_0:
        return TIMER0_BASE;
        case TIMER_1:
        return TIMER1_BASE;
        case TIMER_2:
        return TIMER2_BASE;
        case TIMER_3:
        return TIMER3_BASE;
        case TIMER_4:
        return TIMER4_BASE;
        case TIMER_5:
        return TIMER5_BASE;
        default:
        return -1;
    }
}

static int get_timer_num(tim_t dev) {
    switch(dev) {
        case TIMER_0:
        return 0;
        case TIMER_1:
        return 1;
        case TIMER_2:
        return 2;
        case TIMER_3:
        return 3;
        case TIMER_4:
        return 4;
        case TIMER_5:
        return 5;
        default:
        return -1;
    }
}

static IRQn_Type get_timer_irq(tim_t dev) {
    switch(dev) {
        case TIMER_0:
        return TIMER0A_IRQn;
        case TIMER_1:
        return TIMER1A_IRQn;
        case TIMER_2:
        return TIMER2A_IRQn;
        case TIMER_3:
        return TIMER3A_IRQn;
        case TIMER_4:
        return TIMER4A_IRQn;
        case TIMER_5:
        return TIMER5A_IRQn;
        default:
        return -1;
    }
}

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
    DEBUG("timer_init(%d)\n", dev);

    if (dev == TIMER_SYSTICK) {
        ROM_SysTickPeriodSet(HWTIMER_MAXTICKS / (HWTIMER_PRESCALE + 1));
        ROM_SysTickEnable();
    } else {
        ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0 + get_timer_num(dev));
        config[get_timer_num(dev)].cb = callback;
    }

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
    DEBUG("timer_set(%d, %d)\n", dev, timeout);

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
    DEBUG("timer_set_absolute(%d, %d)\n", dev, value);
    int timer = get_timer_base(dev);
    IRQn_Type irq = get_timer_irq(dev);


    DEBUG("IRQn_Type = %d\n", irq);

    ROM_TimerConfigure(timer, TIMER_MODE);
    ROM_TimerPrescaleSet(timer, TIMER_A, HWTIMER_PRESCALE);
    ROM_TimerLoadSet(timer, TIMER_A, value);

    //
    // Setup the interrupts for the timer timeouts.
    //
    NVIC_EnableIRQ(irq);
    ROM_TimerIntEnable(timer, TIMER_MODE);

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
    DEBUG("timer_clear(%d, %d)\n", dev, channel);
    ROM_TimerIntDisable(get_timer_base(dev), TIMER_MODE);

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
    uint32_t value;
    if (dev == TIMER_SYSTICK)
        value = ROM_SysTickValueGet();
    else
        value = ROM_TimerValueGet(get_timer_base(dev), TIMER_A);

    DEBUG("timer_read(%d) = %d\n", dev, value);
    return value;
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
    ROM_TimerEnable(get_timer_base(dev), TIMER_MODE);
}

/**
 * @brief Disable interrupts for the given timer
 *
 * @param[in] dev           the timer to disable interrupts for
 */
void timer_irq_disable(tim_t dev) {
    ROM_TimerDisable(get_timer_base(dev), TIMER_MODE);
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
    ((TIMER0_Type *) get_timer_base(dev))->TAV = 0;
}

#if TIMER_0_EN
void TIMER_0_ISR(void)
{
    ISR_ENTER();
    irq_handler(TIMER_0, TIMER_0_DEV);
    ISR_EXIT();
}
#endif
#if TIMER_1_EN
__attribute__ ((naked)) void TIMER_1_ISR(void)
{
    ISR_ENTER();
    irq_handler(TIMER_1, TIMER_1_DEV);
    ISR_EXIT();
}
#endif
#if TIMER_2_EN
__attribute__ ((naked)) void TIMER_2_ISR(void)
{
    ISR_ENTER();
    irq_handler(TIMER_2, TIMER_2_DEV);
    ISR_EXIT();
}
#endif
#if TIMER_3_EN
__attribute__ ((naked)) void TIMER_3_ISR(void)
{
    ISR_ENTER();
    irq_handler(TIMER_3, TIMER_3_DEV);
    ISR_EXIT();
}
#endif
#if TIMER_4_EN
__attribute__ ((naked)) void TIMER_4_ISR(void)
{
    ISR_ENTER();
    irq_handler(TIMER_4, TIMER_4_DEV);
    ISR_EXIT();
}
#endif
#if TIMER_5_EN
__attribute__ ((naked)) void TIMER_5_ISR(void)
{
    ISR_ENTER();
    irq_handler(TIMER_5, TIMER_5_DEV);
    ISR_EXIT();
}
#endif

static inline void irq_handler(tim_t timer, TIMER0_Type *dev)
{
    ROM_TimerIntClear((uint32_t) dev, TIMER_TIMA_TIMEOUT);

    config[timer].cb(get_timer_num(timer));

    if (sched_context_switch_request) {
        thread_yield();
    }
}