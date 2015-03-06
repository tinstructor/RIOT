/*
 * Copyright (C) 2015 volatiles UG (haftungsbeschränkt)
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
 * @brief       Low-level SPI driver implementation
 *
 * @author      Benjamin Valentin <benjamin.valentin@volatiles.de>
 *
 * @}
 */

#include "tm4c123gh6pm.h"
#include "periph/spi.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"

/* guard this file in case no SPI device is defined */
#if SPI_NUMOF

int spi_init_master(spi_t dev, spi_conf_t conf, spi_speed_t speed) {

	uint32_t _speed;
	switch (speed) {
		case SPI_SPEED_100KHZ:
			_speed = 100000;
			break;
		case SPI_SPEED_400KHZ:
			_speed = 400000;
			break;
		case SPI_SPEED_1MHZ:
			_speed = 1000000;
			break;
		case SPI_SPEED_5MHZ:
			_speed = 5000000;
			break;
		case SPI_SPEED_10MHZ:
			_speed = 10000000;
			break;
	}

	uint32_t _dev;
	switch (dev) {
		case SPI_0:
			_dev = SSI0_BASE;
			break;
		case SPI_1:
			_dev = SSI1_BASE;
			break;
		case SPI_2:
			_dev = SSI2_BASE;
			break;
		case SPI_3:
			_dev = SSI3_BASE;
			break;
	}

	spi_conf_pins(dev);
	ROM_SSIConfigSetExpClk(_dev, ROM_SysCtlClockGet(), conf, SSI_MODE_MASTER, _speed, 8);
	ROM_SSIEnable(_dev);
}

int spi_init_slave(spi_t dev, spi_conf_t conf, char (*cb)(char data)) {

}

int spi_conf_pins(spi_t dev) {
	switch (dev) {
#if SPI_0_EN
		case SPI_0:
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

			ROM_GPIOPinConfigure(GPIO_PA2_SSI0RX);
			ROM_GPIOPinConfigure(GPIO_PA4_SSI0TX);
			ROM_GPIOPinConfigure(GPIO_PA5_SSI0CLK);

			ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_3);
			ROM_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5);
			break;
#endif
#if SPI_1_EN
		case SPI_1:
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

			ROM_GPIOPinConfigure(GPIO_PF0_SSI1RX);
			ROM_GPIOPinConfigure(GPIO_PF1_SSI1TX);
			ROM_GPIOPinConfigure(GPIO_PF2_SSI1CLK);

			ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);
			ROM_GPIOPinTypeSSI(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
			break;
#endif
#if SPI_2_EN
		case SPI_2:
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

			ROM_GPIOPinConfigure(GPIO_PB6_SSI2RX);
			ROM_GPIOPinConfigure(GPIO_PB7_SSI2TX);
			ROM_GPIOPinConfigure(GPIO_PB4_SSI2CLK);

			ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_5);
			ROM_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7);
			break;
#endif
#if SPI_3_EN
		case SPI_3:
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);
			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

			ROM_GPIOPinConfigure(GPIO_PD2_SSI1RX);
			ROM_GPIOPinConfigure(GPIO_PD3_SSI1TX);
			ROM_GPIOPinConfigure(GPIO_PD0_SSI1CLK);

			ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_1);
			ROM_GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);
			break;
#endif
	}

}

int spi_acquire(spi_t dev) {

}

int spi_release(spi_t dev) {

}

int spi_transfer_byte(spi_t dev, char out, char *in) {

}

int spi_transfer_bytes(spi_t dev, char *out, char *in, unsigned int length) {

}

int spi_transfer_reg(spi_t dev, uint8_t reg, char out, char *in) {

}

int spi_transfer_regs(spi_t dev, uint8_t reg, char *out, char *in, unsigned int length) {

}

void spi_transmission_begin(spi_t dev, char reset_val) {

}

void spi_poweron(spi_t dev) {

}

void spi_poweroff(spi_t dev) {

}


#endif /* SPI_NUMOF */