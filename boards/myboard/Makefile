MODULE =$(BOARD)_base

INCLUDES += -I$(RIOTBOARD)/$(BOARD)/drivers/include
DIRS = drivers

all: $(BINDIR)$(MODULE).a
	@echo "		MYBOARD MAKEFILE"
	@echo "		DIRS: ${DIRS}"
	@for i in $(DIRS) ; do "$(MAKE)" -C $$i ; done ;

include $(RIOTBASE)/Makefile.base

clean::
	@for i in $(DIRS) ; do "$(MAKE)" -C $$i clean ; done ;
