/*
 * Copyright (C) 2019 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief       Test for low-level Watchdog driver
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 *
 * @}
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "cpu.h"
#include "periph/wdt.h"
#include "xtimer.h"

#define COUNT_TEST 5

static uint32_t magic  __attribute__((section(".noinit")));
static uint8_t test_state  __attribute__((section(".noinit")));
static uint8_t kick_count  __attribute__((section(".noinit")));

void wdt_init(void); // remove me

static void _test_normal(unsigned int period)
{
    printf("Normal Mode, %u ms\n", period);

    wdt_setup_reboot(0, period);
    wdt_start();

    for (kick_count = 0; kick_count < COUNT_TEST; ++kick_count) {
        puts("kicking Watchdog.");
        wdt_kick();
        xtimer_usleep((period / 2) * 1000);
    }

    puts("not kicking the WTD anymore.");
    xtimer_usleep((period / 2) * 1000);

    while (1) {
        puts("board should reset now.");
        xtimer_usleep(100 * 1000);
    }
}

static void _test_window(unsigned int window, unsigned period) {
    printf("Window Mode, %u < x < %u ms\n", window, period);

    wdt_setup_reboot(window, period);
    wdt_start();

    for (kick_count = 0; kick_count < COUNT_TEST; ++kick_count) {
        xtimer_usleep((window * 3) * 1000 / 2);
        puts("kicking Watchdog.");
        wdt_kick();
    }

    puts("kicking Watchdog early.");
    wdt_kick();
    wdt_kick();
    wdt_kick();

    puts("You should not see this!");
}

static void _early_warning(void* arg)
{
    uint8_t* count = arg;

    if (*count < COUNT_TEST) {
        puts("kicking watchdog.");
        wdt_kick();
        *count += 1;
    } else
        puts("starving watchdog.");
}

static void _test_callback(unsigned int period)
{
    printf("Early Warning Mode, %u ms\n", period);

    kick_count = 0;
    wdt_setup_reboot_with_callback(0, period, _early_warning, &kick_count);
    wdt_start();

    thread_sleep();
}

int main(void)
{

    if (magic != 0xDEADBEEF) {
        magic = 0xDEADBEEF;
        test_state = 0;
        kick_count = COUNT_TEST;
    }

    if (kick_count != COUNT_TEST) {
        puts("ERROR - last test failed.");
        magic = 0;
        thread_sleep();
    }

    puts("\nRIOT Watchdog low-level driver test");

    switch (test_state++) {
        case 0:
            _test_normal(250);
            break;
        case 1:
             _test_normal(2500);
            break;
        case 2:
            _test_callback(1000);
            break;
        case 3:
            _test_window(100, 2500);
            break;
        default:
            puts("Test done.");
            test_state = 0;
    }

    return 0;
}
