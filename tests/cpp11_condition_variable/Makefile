include ../Makefile.tests_common

# ROM is overflowing for these boards when using
# gcc-arm-none-eabi-4.9.3.2015q2-1trusty1 from ppa:terry.guo/gcc-arm-embedded
# (Travis is using this PPA currently, 2015-06-23)
# Debian jessie libstdc++-arm-none-eabi-newlib-4.8.3-9+4 works fine, though.
# Remove this line if Travis is upgraded to a different toolchain which does
# not pull in all C++ locale code whenever exceptions are used.
BOARD_INSUFFICIENT_MEMORY := i-nucleo-lrwan1 nucleo-f334r8 spark-core \
                             stm32f0discovery stm32l0538-disco

# If you want to add some extra flags when compile c++ files, add these flags
# to CXXEXFLAGS variable
CXXEXFLAGS += -std=c++11

# nucleo-f303k8 doesn't have enough RAM to run the test so we reduce the stack
# size for every thread
ifneq (,$(filter nucleo-f303k8,$(BOARD)))
  CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=512
endif

USEMODULE += cpp11-compat
USEMODULE += xtimer
USEMODULE += timex

include $(RIOTBASE)/Makefile.include
