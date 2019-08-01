# Raspberry Pi GPIO

export SWCLK ?= 21
export SWDIO ?= 20
export RST ?= 16

ifeq (1, $(shell grep ARMv6 /proc/cpuinfo > /dev/null; echo $?))
    # raspi2, 3
    export PERIPH_BASE  = 0x3F000000
    export SPEED_COEFF  = 146203
    export SPEED_OFFSET = 36
else
    # raspi1
    export PERIPH_BASE  = 0x20000000
    export SPEED_COEFF  = 113714
    export SPEED_OFFSET = 28
endif

export OPENOCD_ADAPTER_INIT ?= \
  -c 'interface bcm2835gpio' \
  -c 'bcm2835gpio_peripheral_base $(PERIPH_BASE)' \
  -c 'bcm2835gpio_speed_coeffs $(SPEED_COEFF) $(SPEED_OFFSET)' \
  -c 'bcm2835gpio_swd_nums $(SWCLK) $(SWDIO)' \
  -c 'bcm2835gpio_srst_num $(RST)' \
  -c 'transport select swd'
