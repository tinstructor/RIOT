include ../Makefile.tests_common

# HACL* only compiles on 32bit platforms
BOARD_BLACKLIST := arduino-duemilanove arduino-leonardo \
                   arduino-mega2560 arduino-nano \
                   arduino-uno chronos jiminy-mega256rfr2 mega-xplained \
                   msb-430 msb-430h telosb waspmote-pro wsn430-v1_3b \
                   wsn430-v1_4 z1

BOARD_INSUFFICIENT_MEMORY := i-nucleo-lrwan1 \
                             nucleo-f030r8 nucleo-f031k6 nucleo-f042k6 \
                             nucleo-l031k6 nucleo-l053r8 stm32f0discovery \
                             stm32l0538-disco

TEST_ON_CI_WHITELIST += native


USEPKG += libcose
# By default we use hacl as crypto backend, uncomment to use a different
# crypto backend.
USEMODULE += libcose_crypt_hacl
# USEMODULE += libcose_crypt_c25519
# USEMODULE += libcose_crypt_monocypher
USEMODULE += memarray
USEMODULE += embunit

ifneq (,$(filter libcose_crypt_monocypher,$(USEMODULE)))
  CFLAGS += -DTHREAD_STACKSIZE_MAIN=\(6*THREAD_STACKSIZE_DEFAULT\)
else
  CFLAGS += -DTHREAD_STACKSIZE_MAIN=\(4*THREAD_STACKSIZE_DEFAULT\)
endif

include $(RIOTBASE)/Makefile.include
