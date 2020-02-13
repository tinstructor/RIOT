# name of your application
APPLICATION = timing_control

# If no BOARD is found in the environment, use this default:
BOARD ?= remote-revb

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

BOARD_INSUFFICIENT_MEMORY := arduino-duemilanove arduino-leonardo arduino-nano \
                             arduino-uno

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
USEMODULE += xtimer

# Specify all required features
FEATURES_REQUIRED += periph_gpio
FEATURES_REQUIRED += periph_gpio_irq

include $(RIOTBASE)/Makefile.include
