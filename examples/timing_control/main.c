/*
 * Copyright (C) 2019 Robbe Elsas <robbe.elsas@ugent.be>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Application used for interference testing
 *
 * @author      Robbe Elsas <robbe.elsas@ugent.be>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "thread.h"
#include "mutex.h"
#include "msg.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "xtimer.h"
#include "periph/gpio.h"
#include "periph/uart.h"
#include "periph/pm.h"
#include "timing_control_constants.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

kernel_pid_t pid_tc;
kernel_pid_t pid_btn;
kernel_pid_t pid_ua;

static char stack_tc[THREAD_STACKSIZE_MAIN];
static char stack_btn[THREAD_STACKSIZE_MAIN];
static char stack_ua[THREAD_STACKSIZE_MAIN];

xtimer_ticks32_t last_wup_tc;
xtimer_ticks32_t last_wup_st;
xtimer_ticks32_t last_wup_si;
xtimer_t phy_cfg_timer;

static tc_start_flag_t start_flag = {.can_start = false, .has_started = false};
static tc_pin_cfg_t if_tx_pin_cfg = {.first_pin = TX_TX_PIN, .second_pin = IF_TX_PIN};
static tc_offset_t if_tx_offset = {.offset = IF_TX_OFFSET_US};
static tc_phy_t if_trx_phy = {.phy = SUN_FSK_OM1};
static tc_phy_t if_if_phy = {.phy = SUN_FSK_OM1};
static tc_numtx_t if_numtx = {.numtx = NUM_OF_TX};

static bool set_can_start(void)
{
    bool succes = false;

    mutex_lock(&start_flag.lock);
    if (!start_flag.has_started && !start_flag.can_start) {
        start_flag.can_start = true;
        succes = true;
    }
    mutex_unlock(&start_flag.lock);

    return succes;
}

static bool get_can_start(void)
{
    bool can_start = false;

    mutex_lock(&start_flag.lock);
    if (!start_flag.has_started) {
        can_start = start_flag.can_start;
    }
    mutex_unlock(&start_flag.lock);

    return can_start;
}

static bool set_has_started(void)
{
    bool succes = false;

    mutex_lock(&start_flag.lock);
    if (!start_flag.has_started && start_flag.can_start) {
        start_flag.has_started = true;
        start_flag.can_start = false;
        succes = true;
    }
    mutex_unlock(&start_flag.lock);

    return succes;
}

static bool get_has_started(void)
{
    bool has_started = false;

    mutex_lock(&start_flag.lock);
    has_started = start_flag.has_started;
    mutex_unlock(&start_flag.lock);

    return has_started;
}

static void uart_cb(void *arg, uint8_t data)
{
    (void) arg;

    msg_t msg;
    msg.content.value = data;
    msg_send(&msg, pid_ua);
}

static void gpio_cb(void *arg)
{
    (void) arg;

    msg_t msg;
    msg = msg_start;
    msg_send(&msg, pid_btn);
}

static void *thread_ua_handler(void *arg)
{
    (void) arg;

    msg_t msg;
    msg_t msg_queue[8];
    msg_init_queue(msg_queue, 8);

    while (1) {
        msg_receive(&msg);

        switch ((char)msg.content.value)
        {
            case 's':
                set_can_start();
                break;
            case 'r':
                pm_reboot();
                break;
            case 'n':
                {
                    if (!get_has_started()) {
                        msg_try_receive(&msg);
                        int index = CHAR_TO_INT((char)msg.content.value);
                        if (index >= 0 && index <= 9) {
                            switch (index)
                            {
                                case 0:
                                    {
                                        mutex_lock(&if_numtx.lock);
                                        if_numtx.numtx = 5;
                                        mutex_unlock(&if_numtx.lock);
                                    }
                                    break;
                                case 1:
                                    {
                                        mutex_lock(&if_numtx.lock);
                                        if_numtx.numtx = 10;
                                        mutex_unlock(&if_numtx.lock);
                                    }
                                    break;
                                case 2:
                                    {
                                        mutex_lock(&if_numtx.lock);
                                        if_numtx.numtx = 25;
                                        mutex_unlock(&if_numtx.lock);
                                    }
                                    break;
                                case 3:
                                    {
                                        mutex_lock(&if_numtx.lock);
                                        if_numtx.numtx = 100;
                                        mutex_unlock(&if_numtx.lock);
                                    }
                                    break;
                                case 4:
                                    {
                                        mutex_lock(&if_numtx.lock);
                                        if_numtx.numtx = 250;
                                        mutex_unlock(&if_numtx.lock);
                                    }
                                    break;
                                default:
                                    DEBUG("Index option %d not supported\n", index);
                                    break;
                            }

                        }
                        else {
                            DEBUG("Number of tx index %d out of range\n",index);
                        }
                    }
                    else {
                        DEBUG("Experiment has already started: can't set number of tx\n");
                    }
                }
                break;
            case 't':
                {
                    if (!get_has_started()) {
                        msg_try_receive(&msg);
                        int index = CHAR_TO_INT((char)msg.content.value);
                        if (index >= 0 && index <= NUM_PHY_CFG) {
                            mutex_lock(&if_trx_phy.lock);
                            uint8_t pulses = (NUM_PHY_CFG - if_trx_phy.phy + index) % NUM_PHY_CFG;
                            if_trx_phy.phy = index;
                            last_wup_st = xtimer_now();
                            for (size_t i = 0; i < pulses; i++) {
                                gpio_set(TX_PHY_CFG_PIN);
                                xtimer_periodic_wakeup(&last_wup_st, PULSE_DURATION_US);
                                gpio_clear(TX_PHY_CFG_PIN);
                                gpio_set(RX_PHY_CFG_PIN);
                                xtimer_periodic_wakeup(&last_wup_st, PULSE_DURATION_US);
                                gpio_clear(RX_PHY_CFG_PIN);
                                xtimer_periodic_wakeup(&last_wup_st, PHY_CFG_INTERVAL);
                            }
                            mutex_unlock(&if_trx_phy.lock);
                        }
                        else {
                            DEBUG("TRX PHY index %d out of range\n",index);
                        }
                    }
                    else {
                        DEBUG("Experiment has already started: can't set trx phy\n");
                    }
                }
                break;
            case 'i':
                {
                    if (!get_has_started()) {
                        msg_try_receive(&msg);
                        int index = CHAR_TO_INT((char)msg.content.value);
                        if (index >= 0 && index <= NUM_PHY_CFG) {
                            mutex_lock(&if_if_phy.lock);
                            uint8_t pulses = (NUM_PHY_CFG - if_if_phy.phy + index) % NUM_PHY_CFG;
                            if_if_phy.phy = index;
                            last_wup_si = xtimer_now();
                            for (size_t i = 0; i < pulses; i++) {
                                gpio_set(IF_PHY_CFG_PIN);
                                xtimer_periodic_wakeup(&last_wup_si, PULSE_DURATION_US);
                                gpio_clear(IF_PHY_CFG_PIN);
                                xtimer_periodic_wakeup(&last_wup_si, PHY_CFG_INTERVAL);
                            }
                            mutex_unlock(&if_if_phy.lock);
                        }
                        else {
                            DEBUG("IF PHY index %d out of range\n",index);
                        }
                    }
                    else {
                        DEBUG("Experiment has already started: can't set if phy\n");
                    }
                }
                break;
            case 'o':
                {
                    if (!get_has_started()) {
                        msg_try_receive(&msg);
                        int index = CHAR_TO_INT((char)msg.content.value);
                        if (index >= 0 && index <= 9) {
                            switch (index)
                            {
                                case 0:
                                    {
                                        // NOTE: if payload starts at tx sync
                                        mutex_lock(&if_tx_offset.lock);
                                        if_tx_offset.offset = 1920UL;
                                        mutex_lock(&if_tx_pin_cfg.lock);
                                        if_tx_pin_cfg.first_pin = IF_TX_PIN;
                                        if_tx_pin_cfg.second_pin = TX_TX_PIN;
                                        mutex_unlock(&if_tx_pin_cfg.lock);
                                        mutex_unlock(&if_tx_offset.lock);
                                    }
                                    break;
                                case 1:
                                    {
                                        // NOTE: if and tx center aligned
                                        mutex_lock(&if_tx_offset.lock);
                                        if_tx_offset.offset = 7920UL;
                                        mutex_lock(&if_tx_pin_cfg.lock);
                                        if_tx_pin_cfg.first_pin = TX_TX_PIN;
                                        if_tx_pin_cfg.second_pin = IF_TX_PIN;
                                        mutex_unlock(&if_tx_pin_cfg.lock);
                                        mutex_unlock(&if_tx_offset.lock);
                                    }
                                    break;
                                case 2:
                                    {
                                        // NOTE: if overlaps with FCS of tx
                                        mutex_lock(&if_tx_offset.lock);
                                        if_tx_offset.offset = 15840UL;
                                        mutex_lock(&if_tx_pin_cfg.lock);
                                        if_tx_pin_cfg.first_pin = TX_TX_PIN;
                                        if_tx_pin_cfg.second_pin = IF_TX_PIN;
                                        mutex_unlock(&if_tx_pin_cfg.lock);
                                        mutex_unlock(&if_tx_offset.lock);
                                    }
                                    break;
                                default:
                                    DEBUG("Index option %d not supported\n", index);
                                    break;
                            }
                        }
                    }
                    else {
                        DEBUG("Experiment has already started: can't set offset\n");
                    }
                }
                break;
            default:
                break;
        }
    }

    return NULL;
}

static void *thread_btn_handler(void *arg)
{
    (void) arg;
    msg_t msg;
    msg_receive(&msg);

    set_can_start();

    return NULL;
}

static void *thread_tc_handler(void *arg)
{
    (void) arg;

    msg_t msg;
    msg_t msg_queue[8];
    msg_init_queue(msg_queue, 8);

    uint8_t phy_reconfigs = 0;
    uint8_t experiments = 0;

    while (!get_can_start()) { thread_yield(); };
    set_has_started();
    xtimer_sleep(2);

    mutex_lock(&if_trx_phy.lock);
    mutex_lock(&if_if_phy.lock);
    mutex_lock(&if_numtx.lock);
    msg = msg_phy_cfg;
    last_wup_tc = xtimer_now();
    xtimer_set_msg(&phy_cfg_timer, if_numtx.numtx * TX_WUP_INTERVAL, &msg, thread_getpid());
    while (experiments < NUM_OF_PHY_IF) {
        mutex_lock(&if_tx_offset.lock);
        mutex_lock(&if_tx_pin_cfg.lock);
        xtimer_periodic_wakeup(&last_wup_tc, TX_WUP_INTERVAL - if_tx_offset.offset - PULSE_DURATION_US);
        gpio_set(if_tx_pin_cfg.first_pin);
        xtimer_periodic_wakeup(&last_wup_tc, if_tx_offset.offset);
        gpio_set(if_tx_pin_cfg.second_pin);
        xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US - if_tx_offset.offset);
        gpio_clear(if_tx_pin_cfg.first_pin);
        xtimer_periodic_wakeup(&last_wup_tc, if_tx_offset.offset);
        gpio_clear(if_tx_pin_cfg.second_pin);
        mutex_unlock(&if_tx_pin_cfg.lock);
        mutex_unlock(&if_tx_offset.lock);
        if (msg_try_receive(&msg) == 1) {
            xtimer_periodic_wakeup(&last_wup_tc, WAITING_PERIOD_US);
            gpio_set(TX_PHY_CFG_PIN);
            xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US);
            gpio_clear(TX_PHY_CFG_PIN);
            gpio_set(RX_PHY_CFG_PIN);
            xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US);
            gpio_clear(RX_PHY_CFG_PIN);

            phy_reconfigs++;

            if (phy_reconfigs >= NUM_OF_PHY_TRX) {
                gpio_set(IF_PHY_CFG_PIN);
                xtimer_periodic_wakeup(&last_wup_tc, PULSE_DURATION_US);
                gpio_clear(IF_PHY_CFG_PIN);

                experiments++;
                phy_reconfigs = 0;
            }

            xtimer_periodic_wakeup(&last_wup_tc, WAITING_PERIOD_US);
            xtimer_set_msg(&phy_cfg_timer, if_numtx.numtx * TX_WUP_INTERVAL, &msg, thread_getpid());
        }
    }
    mutex_unlock(&if_numtx.lock);
    mutex_unlock(&if_if_phy.lock);
    mutex_unlock(&if_trx_phy.lock);
    
    return NULL;
}

int main(void)
{
    puts("Welcome to RIOT!");

    /* initialize output pins */
    gpio_init(TX_TX_PIN, GPIO_OUT);
    gpio_init(IF_TX_PIN, GPIO_OUT);
    gpio_init(TX_PHY_CFG_PIN, GPIO_OUT);
    gpio_init(RX_PHY_CFG_PIN, GPIO_OUT);
    gpio_init(IF_PHY_CFG_PIN, GPIO_OUT);

    /* Clear output pins */
    gpio_clear(TX_TX_PIN);
    gpio_clear(IF_TX_PIN);
    gpio_clear(TX_PHY_CFG_PIN);
    gpio_clear(RX_PHY_CFG_PIN);
    gpio_clear(IF_PHY_CFG_PIN);
    
    pid_btn = thread_create(stack_btn, sizeof(stack_btn), 
              THREAD_PRIORITY_MAIN - 1, 0, thread_btn_handler, 
              NULL, "thread btn");

    gpio_init_int(BTN0_PIN, BTN0_MODE, GPIO_RISING, gpio_cb, NULL);

    pid_ua = thread_create(stack_ua, sizeof(stack_ua), 
             THREAD_PRIORITY_MAIN - 1, 0, thread_ua_handler, 
             NULL, "thread ua");

    uart_init(UART_DEV(0), 115200, uart_cb, NULL);

    pid_tc = thread_create(stack_tc, sizeof(stack_tc), 
             THREAD_PRIORITY_MAIN - 1, 0, thread_tc_handler, 
             NULL, "thread tc");
    
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}