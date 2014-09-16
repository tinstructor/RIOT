/*
 * Copyright (C) 2014 Freie Universität Berlin
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
 * @brief       Startup code and interrupt vector definition
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdint.h>

#include "board.h" // for debuging
#include "driverlib/rom.h"

/**
 * memory markers as defined in the linker script
 */
extern uint32_t _sfixed;
extern uint32_t _efixed;
extern uint32_t _etext;
extern uint32_t _srelocate;
extern uint32_t _erelocate;
extern uint32_t _szero;
extern uint32_t _ezero;
extern uint32_t _sstack;
extern uint32_t _estack;

/**
 * @brief functions for initializing the board, std-lib and kernel
 */
extern void board_init(void);
extern void kernel_init(void);
extern void __libc_init_array(void);

/**
 * @brief This function is the entry point after a system reset
 *
 * After a system reset, the following steps are necessary and carried out:
 * 1. load data section from flash to ram
 * 2. overwrite uninitialized data section (BSS) with zeros
 * 3. initialize the newlib
 * 4. initialize the board (sync clock, setup std-IO)
 * 5. initialize and start RIOTs kernel
 */
void reset_handler(void)
{
    uint32_t *dst;
    uint32_t *src = &_etext;

    /* load data section from flash to ram */
    for (dst = &_srelocate; dst < &_erelocate; ) {
        *(dst++) = *(src++);
    }

    /* default bss section to zero */
    for (dst = &_szero; dst < &_ezero; ) {
        *(dst++) = 0;
    }

    /* initialize the board and startup the kernel */
    board_init();
    /* initialize std-c library (this should be done after board_init) */

    __libc_init_array();
    /* startup the kernel */

    BLUE_LED_ON;

    while (1) {
    	asm("nop");
    }
//    kernel_init();
}

/**
 * @brief Default handler is called in case no interrupt handler was defined
 */
void dummy_handler(void)
{
	while(1){
		GREEN_LED_ON;
		ROM_SysCtlDelay(5000000);
		GREEN_LED_OFF;
		ROM_SysCtlDelay(5000000);
	}
}

void isr_nmi(void)
{
	while(1){
		RED_LED_ON;
		ROM_SysCtlDelay(5000000);
		RED_LED_OFF;
		ROM_SysCtlDelay(5000000);
	}
}

void isr_mem_manage(void)
{
	while(1){
		BLUE_LED_ON;
		ROM_SysCtlDelay(5000000);
		BLUE_LED_OFF;
		ROM_SysCtlDelay(5000000);
	}
}

void isr_debug_mon(void)
{
	while(1){
		GREEN_LED_ON;
		BLUE_LED_ON;
		ROM_SysCtlDelay(5000000);
		GREEN_LED_OFF;
		ROM_SysCtlDelay(5000000);
	}
}

void isr_hard_fault(void)
{
	while(1){
		GREEN_LED_ON;
		RED_LED_ON;
		ROM_SysCtlDelay(5000000);
		GREEN_LED_OFF;
		ROM_SysCtlDelay(5000000);
	}
}

void isr_bus_fault(void)
{
	while(1){
		BLUE_LED_ON;
		RED_LED_ON;
		ROM_SysCtlDelay(5000000);
		BLUE_LED_OFF;
		ROM_SysCtlDelay(5000000);
	}
}

void isr_usage_fault(void)
{
	while(1){
		RED_LED_ON;
		BLUE_LED_ON;
		ROM_SysCtlDelay(5000000);
		RED_LED_OFF;
		ROM_SysCtlDelay(5000000);
	}
}

/* Cortex-M specific interrupt vectors */
void isr_svc(void)                  __attribute__ ((weak, alias("dummy_handler")));
void isr_pendsv(void)               __attribute__ ((weak, alias("dummy_handler")));
void isr_systick(void)              __attribute__ ((weak, alias("dummy_handler")));

/* TM4C123 specific interrupt vector */


/* interrupt vector table */
__attribute__ ((section(".vectors")))
const void *interrupt_vector[] = {
    /* Stack pointer */
    (void*) (&_estack),             /* pointer to the top of the empty stack */
    /* Cortex-M4 handlers */
    (void*) reset_handler,          /* entry point of the program */
    (void*) isr_nmi,                /* non maskable interrupt handler */
    (void*) isr_hard_fault,         /* if you end up here its not good */
    (void*) isr_mem_manage,         /* memory controller interrupt */
    (void*) isr_bus_fault,          /* also not good to end up here */
    (void*) isr_usage_fault,        /* autsch */
    (void*) (0UL),                  /* Reserved */
    (void*) (0UL),                  /* Reserved */
    (void*) (0UL),                  /* Reserved */
    (void*) (0UL),                  /* Reserved */
    (void*) isr_svc,                /* system call interrupt */
    (void*) isr_debug_mon,          /* debug interrupt */
    (void*) (0UL),                  /* Reserved */
    (void*) isr_pendsv,             /* pendSV interrupt, used for task switching in RIOT */
    (void*) isr_systick,            /* SysTick interrupt, not used in RIOT */
    /* TI specific peripheral handlers */
    dummy_handler,                      // GPIO Port A
    dummy_handler,                      // GPIO Port B
    dummy_handler,                      // GPIO Port C
    dummy_handler,                      // GPIO Port D
    dummy_handler,                      // GPIO Port E
    dummy_handler,                      // UART0 Rx and Tx
    dummy_handler,                      // UART1 Rx and Tx
    dummy_handler,                      // SSI0 Rx and Tx
    dummy_handler,                      // I2C0 Master and Slave
    dummy_handler,                      // PWM Fault
    dummy_handler,                      // PWM Generator 0
    dummy_handler,                      // PWM Generator 1
    dummy_handler,                      // PWM Generator 2
    dummy_handler,                      // Quadrature Encoder 0
    dummy_handler,                      // ADC Sequence 0
    dummy_handler,                      // ADC Sequence 1
    dummy_handler,                      // ADC Sequence 2
    dummy_handler,                      // ADC Sequence 3
    dummy_handler,                      // Watchdog timer
    dummy_handler,                      // Timer 0 subtimer A
    dummy_handler,                      // Timer 0 subtimer B
    dummy_handler,                      // Timer 1 subtimer A
    dummy_handler,                      // Timer 1 subtimer B
    dummy_handler,                      // Timer 2 subtimer A
    dummy_handler,                      // Timer 2 subtimer B
    dummy_handler,                      // Analog Comparator 0
    dummy_handler,                      // Analog Comparator 1
    dummy_handler,                      // Analog Comparator 2
    dummy_handler,                      // System Control (PLL, OSC, BO)
    dummy_handler,                      // FLASH Control
    dummy_handler,                      // GPIO Port F
    dummy_handler,                      // GPIO Port G
    dummy_handler,                      // GPIO Port H
    dummy_handler,                      // UART2 Rx and Tx
    dummy_handler,                      // SSI1 Rx and Tx
    dummy_handler,                      // Timer 3 subtimer A
    dummy_handler,                      // Timer 3 subtimer B
    dummy_handler,                      // I2C1 Master and Slave
    dummy_handler,                      // Quadrature Encoder 1
    dummy_handler,                      // CAN0
    dummy_handler,                      // CAN1
    dummy_handler,                      // CAN2
    0,                                      // Reserved
    dummy_handler,                      // Hibernate
    dummy_handler,                      // USB0
    dummy_handler,                      // PWM Generator 3
    dummy_handler,                      // uDMA Software Transfer
    dummy_handler,                      // uDMA Error
    dummy_handler,                      // ADC1 Sequence 0
    dummy_handler,                      // ADC1 Sequence 1
    dummy_handler,                      // ADC1 Sequence 2
    dummy_handler,                      // ADC1 Sequence 3
    0,                                      // Reserved
    0,                                      // Reserved
    dummy_handler,                      // GPIO Port J
    dummy_handler,                      // GPIO Port K
    dummy_handler,                      // GPIO Port L
    dummy_handler,                      // SSI2 Rx and Tx
    dummy_handler,                      // SSI3 Rx and Tx
    dummy_handler,                      // UART3 Rx and Tx
    dummy_handler,                      // UART4 Rx and Tx
    dummy_handler,                      // UART5 Rx and Tx
    dummy_handler,                      // UART6 Rx and Tx
    dummy_handler,                      // UART7 Rx and Tx
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    dummy_handler,                      // I2C2 Master and Slave
    dummy_handler,                      // I2C3 Master and Slave
    dummy_handler,                      // Timer 4 subtimer A
    dummy_handler,                      // Timer 4 subtimer B
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    dummy_handler,                      // Timer 5 subtimer A
    dummy_handler,                      // Timer 5 subtimer B
    dummy_handler,                      // Wide Timer 0 subtimer A
    dummy_handler,                      // Wide Timer 0 subtimer B
    dummy_handler,                      // Wide Timer 1 subtimer A
    dummy_handler,                      // Wide Timer 1 subtimer B
    dummy_handler,                      // Wide Timer 2 subtimer A
    dummy_handler,                      // Wide Timer 2 subtimer B
    dummy_handler,                      // Wide Timer 3 subtimer A
    dummy_handler,                      // Wide Timer 3 subtimer B
    dummy_handler,                      // Wide Timer 4 subtimer A
    dummy_handler,                      // Wide Timer 4 subtimer B
    dummy_handler,                      // Wide Timer 5 subtimer A
    dummy_handler,                      // Wide Timer 5 subtimer B
    dummy_handler,                      // FPU
    0,                                      // Reserved
    0,                                      // Reserved
    dummy_handler,                      // I2C4 Master and Slave
    dummy_handler,                      // I2C5 Master and Slave
    dummy_handler,                      // GPIO Port M
    dummy_handler,                      // GPIO Port N
    dummy_handler,                      // Quadrature Encoder 2
    0,                                      // Reserved
    0,                                      // Reserved
    dummy_handler,                      // GPIO Port P (Summary or P0)
    dummy_handler,                      // GPIO Port P1
    dummy_handler,                      // GPIO Port P2
    dummy_handler,                      // GPIO Port P3
    dummy_handler,                      // GPIO Port P4
    dummy_handler,                      // GPIO Port P5
    dummy_handler,                      // GPIO Port P6
    dummy_handler,                      // GPIO Port P7
    dummy_handler,                      // GPIO Port Q (Summary or Q0)
    dummy_handler,                      // GPIO Port Q1
    dummy_handler,                      // GPIO Port Q2
    dummy_handler,                      // GPIO Port Q3
    dummy_handler,                      // GPIO Port Q4
    dummy_handler,                      // GPIO Port Q5
    dummy_handler,                      // GPIO Port Q6
    dummy_handler,                      // GPIO Port Q7
    dummy_handler,                      // GPIO Port R
    dummy_handler,                      // GPIO Port S
    dummy_handler,                      // PWM 1 Generator 0
    dummy_handler,                      // PWM 1 Generator 1
    dummy_handler,                      // PWM 1 Generator 2
    dummy_handler,                      // PWM 1 Generator 3
    dummy_handler                       // PWM 1 Fault
};
