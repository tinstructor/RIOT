/*
 * Copyright (C) 2019 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_samd5x
 * @{
 *
 * @file        cpu.c
 * @brief       Implementation of the CPU initialization for Microchip SAMD5x/SAME5x MCUs
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 * @}
 */

#include "cpu.h"
#include "periph_conf.h"
#include "periph/init.h"

static void xosc32k_init(void)
{
    OSC32KCTRL->XOSC32K.reg = OSC32KCTRL_XOSC32K_ENABLE | OSC32KCTRL_XOSC32K_XTALEN
                | OSC32KCTRL_XOSC32K_EN32K | OSC32KCTRL_XOSC32K_RUNSTDBY
                | OSC32KCTRL_XOSC32K_STARTUP(7);

    while (!OSC32KCTRL->STATUS.bit.XOSC32KRDY) {}

    GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_XOSC32K)
                 | GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_GENEN;
}

static void fdpll0_init(uint32_t f_cpu)
{
    uint32_t mul = f_cpu << 4;
    mul += 32768 / 2;
    mul /= 32768;

    OSCCTRL->Dpll[0].DPLLRATIO.reg = OSCCTRL_DPLLRATIO_LDRFRAC(mul & 0xF)
                       | OSCCTRL_DPLLRATIO_LDR((mul >> 4) - 1);

    OSCCTRL->Dpll[0].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_REFCLK_XOSC32
                       | OSCCTRL_DPLLCTRLB_WUF
                       | OSCCTRL_DPLLCTRLB_LBYPASS;

    GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0].reg = GCLK_PCHCTRL_GEN(1)
                          | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0].reg & GCLK_PCHCTRL_CHEN)) {}

    GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL032K].reg = GCLK_PCHCTRL_GEN(1)
                             | GCLK_PCHCTRL_CHEN;
    while (!(GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL032K].reg & GCLK_PCHCTRL_CHEN)) {}

    OSCCTRL->Dpll[0].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;

    while (!(OSCCTRL->Dpll[0].DPLLSTATUS.bit.CLKRDY &&
             OSCCTRL->Dpll[0].DPLLSTATUS.bit.LOCK)) {}

    GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_DPLL0)
                         | GCLK_GENCTRL_GENEN;
}

/* clock used by xtimer */
static void gclk_1mhz_init(uint32_t f_cpu) {

    GCLK->GENCTRL[5].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_DPLL0)
                         | GCLK_GENCTRL_DIV(f_cpu / 1000000)
                         | GCLK_GENCTRL_GENEN;
}

/**
 * @brief Initialize the CPU, set IRQ priorities, clocks
 */
void cpu_init(void)
{
    /* initialize the Cortex-M core */
    cortexm_init();

    /* turn on only needed APB peripherals */
    MCLK->APBAMASK.reg = MCLK_APBAMASK_MCLK
                         | MCLK_APBAMASK_OSCCTRL
                         | MCLK_APBAMASK_OSC32KCTRL
                         | MCLK_APBAMASK_GCLK
#ifdef MODULE_PERIPH_GPIO_IRQ
                         | MCLK_APBAMASK_EIC
#endif
                         ;
#ifdef MODULE_PERIPH_GPIO
    MCLK->APBBMASK.reg = MCLK_APBBMASK_PORT;
#endif

    /* enable the Cortex M Cache Controller */
    CMCC->CTRL.bit.CEN = 1;

    /* Software reset the GCLK module to ensure it is re-initialized correctly */
    GCLK->CTRLA.reg = GCLK_CTRLA_SWRST;
    while (GCLK->CTRLA.reg & GCLK_CTRLA_SWRST) {}
    while (GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_SWRST) {}

    xosc32k_init();
    fdpll0_init(CLOCK_CORECLOCK);
    gclk_1mhz_init(CLOCK_CORECLOCK);

    while (GCLK->SYNCBUSY.reg) {}

#ifdef MODULE_PERIPH_PM
    /* enable power managemet module */
    MCLK->APBAMASK.reg |= MCLK_APBAMASK_PM;
#endif

    /* trigger static peripheral initialization */
    periph_init();
}
