export APPLICATION = can_trx
include ../Makefile.tests_common

BOARD_INSUFFICIENT_MEMORY := arduino-duemilanove arduino-leonardo arduino-nano \
                             arduino-uno

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

USEMODULE += can_trx

TRX_TO_BUILD ?= tja1042 ncv7356

ifneq (,$(filter tja1042,$(TRX_TO_BUILD)))
  USEMODULE += tja1042
endif

ifneq (,$(filter ncv7356,$(TRX_TO_BUILD)))
  USEMODULE += ncv7356
endif

include $(RIOTBASE)/Makefile.include
