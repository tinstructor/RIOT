/*
 * Copyright (C) 2019 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_sam0_common
 * @ingroup     drivers_periph_wdt
 * @{
 *
 * @file        wdt.c
 * @brief       Low-level WDT driver implementation
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 *
 * @}
 */

#include <stdint.h>
#include "periph/wdt.h"
#include "board.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#ifndef WDT_CLOCK_HZ
#define WDT_CLOCK_HZ 1024
#endif

/* work around inconsistency in header files */
#ifndef WDT_CONFIG_PER_8_Val
#define WDT_CONFIG_PER_8_Val WDT_CONFIG_PER_CYC8_Val
#endif
#ifndef WDT_CONFIG_PER_8K_Val
#define WDT_CONFIG_PER_8K_Val WDT_CONFIG_PER_CYC8192_Val
#endif
#ifndef WDT_CONFIG_PER_16K_Val
#define WDT_CONFIG_PER_16K_Val WDT_CONFIG_PER_CYC16384_Val
#endif

static wdt_cb_t cb;
static void* cb_arg;

static inline void _set_enable(bool on)
{
#ifdef WDT_CTRLA_ENABLE
    WDT->CTRLA.bit.ENABLE = on;
#else
    WDT->CTRL.bit.ENABLE = on;
#endif
}

static inline bool _is_enabled(void)
{
#ifdef WDT_CTRLA_ENABLE
    return WDT->CTRLA.bit.ENABLE;
#else
    return WDT->CTRL.bit.ENABLE;
#endif
}

static inline void _wait_syncbusy(void)
{
#ifdef WDT_STATUS_SYNCBUSY
    while (WDT->STATUS.bit.SYNCBUSY) {}
#else
    while (WDT->SYNCBUSY.reg) {}
#endif
}

static uint32_t ms_to_per(uint32_t ms)
{
    const uint32_t cycles = (ms * WDT_CLOCK_HZ) / 1000;

    /* Minimum WDT period is 8 clock cycles (register value 0) */
    if (cycles <= 8) {
        return 0;
    }

    /* Round up to next pow2 and calculate the register value */
    return 29 - __builtin_clz(cycles - 1);
}

#ifdef CPU_SAMD21
static void _wdt_clock_setup(void)
{
/* RTC / RTT will alredy set up GCLK2 as needed */
#if !defined(USEMODULE_PERIPH_RTC) && !defined(USEMODULE_PERIPH_RTT)
    /* Setup clock GCLK2 with OSCULP32K divided by 32 */
    GCLK->GENDIV.reg  = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(4);
    GCLK->GENCTRL.reg = GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_DIVSEL;

    while (GCLK->STATUS.bit.SYNCBUSY) {}
#endif

    /* Connect to GCLK2 (~1.024 kHz) */
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_WDT
                      | GCLK_CLKCTRL_GEN_GCLK2
                      | GCLK_CLKCTRL_CLKEN;
}
#else
static void _wdt_clock_setup(void)
{
    /* nothing to do here */
}
#endif

void wdt_init(void)
{
    _wdt_clock_setup();
#ifdef MCLK
    MCLK->APBAMASK.bit.WDT_ = 1;
#else
    PM->APBAMASK.bit.WDT_ = 1;
#endif

    _set_enable(0);
    NVIC_EnableIRQ(WDT_IRQn);
}

void wdt_setup_reboot_with_callback(uint32_t period_min, uint32_t period_max,
                                    wdt_cb_t wdt_callback, void *arg)
{
    uint32_t per, win;

    if (period_max == 0) {
        DEBUG("invalid period: period_max = %lu\n", period_max);
        return;
    }

    per = ms_to_per(period_max);

    if (per == WDT_CONFIG_PER_8_Val && wdt_callback) {
        DEBUG("period too short for early warning\n");
        return;
    }

    if (per > WDT_CONFIG_PER_16K_Val) {
        DEBUG("invalid period: period_max = %lu\n", period_max);
        return;
    }

    if (period_min) {
        win = ms_to_per(period_min);

        if (win > WDT_CONFIG_PER_8K_Val) {
            DEBUG("invalid period: period_min = %lu\n", period_min);
            return;
        }

        if (per < win) {
            per = win + 1;
        }

#ifdef WDT_CTRLA_WEN
        WDT->CTRLA.bit.WEN = 1;
#else
        WDT->CTRL.bit.WEN = 1;
#endif
    } else {
        win = 0;
#ifdef WDT_CTRLA_WEN
        WDT->CTRLA.bit.WEN = 0;
#else
        WDT->CTRL.bit.WEN = 0;
#endif
    }

    cb = wdt_callback;
    cb_arg = arg;

    if (cb != NULL) {
        uint32_t warning_offset = ms_to_per(WDT_WARNING_PERIOD);

        if (warning_offset == 0) {
            warning_offset = 1;
        }

        if (warning_offset >= per) {
            warning_offset = per - 1;
        }

        WDT->INTENSET.reg = WDT_INTENSET_EW;
        WDT->EWCTRL.bit.EWOFFSET = per - warning_offset;
    } else {
        WDT->INTENCLR.reg = WDT_INTENCLR_EW;
    }

    WDT->INTFLAG.reg = WDT_INTFLAG_EW;

    DEBUG("watchdog window: %lx, period: %lx\n", win, per);

    WDT->CONFIG.reg = WDT_CONFIG_WINDOW(win) | WDT_CONFIG_PER(per);
    _wait_syncbusy();

    return;
}

void wdt_setup_reboot(uint32_t min_time, uint32_t max_time)
{
    wdt_setup_reboot_with_callback(min_time, max_time, NULL, NULL);
}

void wdt_stop(void)
{
    _set_enable(0);
    _wait_syncbusy();
}

void wdt_start(void)
{
    _set_enable(1);
    _wait_syncbusy();
}

void wdt_kick(void)
{
    WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY_Val;
}

void isr_wdt(void)
{
    WDT->INTFLAG.reg = WDT_INTFLAG_EW;

    if (cb != NULL) {
        cb(cb_arg);
    }

    cortexm_isr_end();
}
