# tell the Makefile.base which module to build
MODULE = $(BOARD)_base

PART=TM4C123GH6PM
TIVA=/home/benpicco/dev/TivaWare_C_Series-2.0.1.11577

INCLUDES += -I$(TIVA)
CFLAGS +=-DTARGET_IS_BLIZZARD_RB1 -DPART_${PART}

include $(RIOTBASE)/Makefile.base
