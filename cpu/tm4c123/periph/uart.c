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
 * @brief       Low-level UART driver implementation
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Fabian Nack <nack@inf.fu-berlin.de>
 *
 * @}
 */

#include "cpu.h"
#include "tm4c123gh6pm.h"
#include "thread.h"
#include "sched.h"
#include "periph_conf.h"
#include "periph/uart.h"

#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"

/* guard file in case no UART device was specified */
#if UART_NUMOF

/**
 * @brief Each UART device has to store two callbacks.
 */
typedef struct {
    uart_rx_cb_t rx_cb;
    uart_tx_cb_t tx_cb;
    void *arg;
} uart_conf_t;

/**
 * @brief Unified interrupt handler for all UART devices
 *
 * @param uartnum       the number of the UART that triggered the ISR
 * @param uart          the UART device that triggered the ISR
 */
// static inline void irq_handler(uart_t uartnum, USART_TypeDef *uart);


//*****************************************************************************
//
// The list of UART peripherals.
//
//*****************************************************************************
static const uint32_t g_ui32UARTPeriph[3] =
{
    SYSCTL_PERIPH_UART0, SYSCTL_PERIPH_UART1, SYSCTL_PERIPH_UART2
};

static const uint32_t g_ui32UARTBase[3] =
{
    UART0_BASE, UART1_BASE, UART2_BASE
};

//*****************************************************************************
//
//! Configures the UART console.
//!
//! \param ui32PortNum is the number of UART port to use for the serial console
//! (0-2)
//! \param ui32Baud is the bit rate that the UART is to be configured to use.
//! \param ui32SrcClock is the frequency of the source clock for the UART
//! module.
//!
//! This function will configure the specified serial port to be used as a
//! serial console.  The serial parameters are set to the baud rate
//! specified by the \e ui32Baud parameter and use 8 bit, no parity, and 1 stop
//! bit.
//!
//! This function must be called prior to using any of the other UART console
//! functions: UARTprintf() or UARTgets().  This function assumes that the
//! caller has previously configured the relevant UART pins for operation as a
//! UART rather than as GPIOs.
//!
//! \return None.
//
//*****************************************************************************
void
UARTStdioConfig(uint32_t ui32PortNum, uint32_t ui32Baud, uint32_t ui32SrcClock)
{
    //
    // Select the base address of the UART.
    //
    int g_ui32Base = g_ui32UARTBase[ui32PortNum];

    //
    // Enable the UART peripheral for use.
    //
    ROM_SysCtlPeripheralEnable(g_ui32UARTPeriph[ui32PortNum]);

    //
    // Configure the UART for 115200, n, 8, 1
    //
    ROM_UARTConfigSetExpClk(g_ui32Base, ui32SrcClock, ui32Baud,
                            (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_WLEN_8));

   //
    // Enable the UART operation.
    //
    ROM_UARTEnable(g_ui32Base);
}

/**
 * @brief Allocate memory to store the callback functions.
 */
static uart_conf_t uart_config[UART_NUMOF];

int uart_init(uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, uart_tx_cb_t tx_cb, void *arg)
{
    /* do basic initialization */
    int res = uart_init_blocking(uart, baudrate);
    if (res < 0) {
        return res;
    }

    /* remember callback addresses */
    uart_config[uart].rx_cb = rx_cb;
    uart_config[uart].tx_cb = tx_cb;
    uart_config[uart].arg = arg;

    /* enable receive interrupt */
    // TODO

    return 0;
}

int uart_init_blocking(uart_t uart, uint32_t baudrate)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIOA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    ROM_UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, baudrate, 16000000);

    return 0;
}

void uart_tx_begin(uart_t uart)
{

}

int uart_write(uart_t uart, char data)
{
    ROM_UARTCharPutNonBlocking(UART0_BASE, data);

    return 0;
}

int uart_read_blocking(uart_t uart, char *data)
{
    *data = ROM_UARTCharGet(UART0_BASE);

    return 1;
}

int uart_write_blocking(uart_t uart, char data)
{
    ROM_UARTCharPut(UART0_BASE, data);

    return 1;
}

void uart_poweron(uart_t uart)
{

}

void uart_poweroff(uart_t uart)
{

}
/*

#if UART_0_EN
__attribute__((naked)) void UART_0_ISR(void)
{
    ISR_ENTER();
    irq_handler(UART_0, UART_0_DEV);
    ISR_EXIT();
}
#endif

#if UART_1_EN
__attribute__((naked)) void UART_1_ISR(void)
{
    ISR_ENTER();
    irq_handler(UART_1, UART_1_DEV);
    ISR_EXIT();
}
#endif

#if UART_2_EN
__attribute__((naked)) void UART_2_ISR(void)
{
    ISR_ENTER();
    irq_handler(UART_2, UART_2_DEV);
    ISR_EXIT();
}
#endif

static inline void irq_handler(uint8_t uartnum, UART_TypeDef *dev)
{
    if (dev->SR & USART_SR_RXNE) {
        char data = (char)dev->DR;
        uart_config[uartnum].rx_cb(uart_config[uartnum].arg, data);
    }
    else if (dev->SR & USART_SR_TXE) {
        if (uart_config[uartnum].tx_cb(uart_config[uartnum].arg) == 0) {
            dev->CR1 &= ~(USART_CR1_TXEIE);
        }
    }
    if (sched_context_switch_request) {
        thread_yield();
    }
}
*/

#endif /* UART_NUMOF */
