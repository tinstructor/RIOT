# define the module that is build
MODULE = cpu

# add a list of subdirectories, that should also be build
DIRS = periph $(CORTEX_M4_COMMON)

PART=TM4C123GH6PM
TIVA=/home/benpicco/dev/TivaWare_C_Series-2.0.1.11577

INCLUDES += -I$(TIVA)
CFLAGS +=-DTARGET_IS_BLIZZARD_RB1 -DPART_${PART}

include $(RIOTBASE)/Makefile.base
