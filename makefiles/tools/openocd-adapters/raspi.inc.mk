# Raspberry Pi GPIO as debug adapter

SWCLK ?= 21
SWDIO ?= 20
RST ?= 16

ifeq (1, $(shell grep ARMv6 /proc/cpuinfo > /dev/null; echo $?))
    # raspi2, 3
    PERIPH_BASE  = 0x3F000000
    SPEED_COEFF  = 146203
    SPEED_OFFSET = 36
else
    # raspi1
    PERIPH_BASE  = 0x20000000
    SPEED_COEFF  = 113714
    SPEED_OFFSET = 28
endif

OPENOCD_ADAPTER_INIT ?= \
  -c 'interface bcm2835gpio' \
  -c 'bcm2835gpio_peripheral_base $(PERIPH_BASE)' \
  -c 'bcm2835gpio_speed_coeffs $(SPEED_COEFF) $(SPEED_OFFSET)' \
  -c 'bcm2835gpio_swd_nums $(SWCLK) $(SWDIO)' \
  -c 'bcm2835gpio_srst_num $(RST)' \
  -c 'transport select swd'

export OPENOCD_ADAPTER_INIT
