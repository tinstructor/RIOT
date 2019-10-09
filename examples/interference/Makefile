# name of your application
APPLICATION = interference

# If no BOARD is found in the environment, use this default:
BOARD ?= openmote-b

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

BOARD_PROVIDES_NETIF := acd52832 airfy-beacon b-l072z-lrwan1 cc2538dk fox \
        iotlab-m3 iotlab-a8-m3 lobaro-lorabox lsn50 mulle microbit msba2 \
        native nrf51dk nrf51dongle nrf52dk nrf52840dk nrf52840-mdk nrf6310 \
        nucleo-f767zi openmote-b openmote-cc2538 pba-d-01-kw2x remote-pa \
        remote-reva remote-revb samr21-xpro spark-core telosb \
        yunjia-nrf51822 z1

BOARD_WHITELIST += $(BOARD_PROVIDES_NETIF)

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
# gnrc is a meta module including all required, basic gnrc networking modules
USEMODULE += gnrc
# use the default network interface for the board
USEMODULE += gnrc_netdev_default
# automatically initialize the network interface
USEMODULE += auto_init_gnrc_netif
# the application dumps received packets to stdout
USEMODULE += gnrc_pktdump
# layer 2 statistics
USEMODULE += netstats_l2

# We use only the lower layers of the GNRC network stack, hence, we can
# reduce the size of the packet buffer a bit
CFLAGS += -DGNRC_PKTBUF_SIZE=512

# Specify all required features
FEATURES_REQUIRED += periph_gpio
FEATURES_REQUIRED += periph_gpio_irq

include $(RIOTBASE)/Makefile.include

# Set a custom channel if needed
include $(RIOTMAKE)/default-channel.inc.mk
