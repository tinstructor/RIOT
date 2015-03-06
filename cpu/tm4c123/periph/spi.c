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

#include "mutex.h"

#include "tm4c123gh6pm.h"
#include "periph/spi.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"

/* guard this file in case no SPI device is defined */
#if SPI_NUMOF

/**
 * @brief Array holding one pre-initialized mutex for each SPI device
 */
static mutex_t locks[] =  {
#if SPI_0_EN
	[SPI_0] = MUTEX_INIT,
#endif
#if SPI_1_EN
	[SPI_1] = MUTEX_INIT,
#endif
#if SPI_2_EN
	[SPI_2] = MUTEX_INIT
#endif
#if SPI_3_EN
	[SPI_3] = MUTEX_INIT
#endif
};

static inline uint32_t get_spi_dev(spi_t dev) {
	switch (dev) {
		case SPI_0:
			return SSI0_BASE;
		case SPI_1:
			return SSI1_BASE;
		case SPI_2:
			return SSI2_BASE;
		case SPI_3:
			return SSI3_BASE;
		default:
			return 0;
		}
}

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
		default:
			return -2;
	}

	uint32_t _dev = get_spi_dev(dev);
	if (_dev == 0)
		return -1;

	spi_conf_pins(dev);
	ROM_SSIConfigSetExpClk(_dev, ROM_SysCtlClockGet(), conf, SSI_MODE_MASTER, _speed, 8);
	ROM_SSIEnable(_dev);

	return 0;
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
	if (dev >= SPI_NUMOF) {
		return -1;
	}
	mutex_lock(&locks[dev]);
	return 0;
}

int spi_release(spi_t dev) {
	if (dev >= SPI_NUMOF) {
		return -1;
	}
	mutex_unlock(&locks[dev]);
	return 0;
}

int spi_transfer_byte(spi_t dev, char out, char *in) {
	uint32_t data_in;
	uint32_t _dev = get_spi_dev(dev);

	if (_dev == 0)
		return -1;

	ROM_SSIDataPut(_dev, out);
	ROM_SSIDataGet(_dev, &data_in)

	if (in)
		*in = (char) (data_in & 0xff);

	return 1;
}

int spi_transfer_bytes(spi_t dev, char *out, char *in, unsigned int length) {
	uint32_t data_in;
	uint32_t data_out = 0;

	uint32_t _dev = get_spi_dev(dev);

	if (_dev == 0)
		return -1;

	for (unsigned int i = 0; i < length; ++i) {
		if (out)
			data_out = out[i];

		ROM_SSIDataPut(_dev, data_out);
		ROM_SSIDataGet(_dev, &data_in)

		if (in)
			in[i] = data_in;
	}

	return length;
}

int spi_transfer_reg(spi_t dev, uint8_t reg, char out, char *in) {
	int trans_ret;

	trans_ret = spi_transfer_byte(dev, reg, in);
	if (trans_ret < 0) {
		return -1;
	}
	trans_ret = spi_transfer_byte(dev, out, in);
	if (trans_ret < 0) {
		return -1;
	}

	return 1;
}

int spi_transfer_regs(spi_t dev, uint8_t reg, char *out, char *in, unsigned int length) {
	int trans_ret;

	trans_ret = spi_transfer_byte(dev, reg, in);
	if (trans_ret < 0) {
		return -1;
	}
	trans_ret = spi_transfer_bytes(dev, out, in, length);
	if (trans_ret < 0) {
		return -1;
	}

	return trans_ret;
}

void spi_transmission_begin(spi_t dev, char reset_val) {

}

void spi_poweron(spi_t dev) {

}

void spi_poweroff(spi_t dev) {

}


#endif /* SPI_NUMOF */