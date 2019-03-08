SCREENSIZE  := text
NUM_MODELS  ?= 10
FILESYSTEMS := common text

TARGET = emu_devof7
FAMILY = devo

SRC += $(SDIR)/target/tx/devo/devof7/ia9211_map.c
CFLAGS +=-I$(SDIR)/target/drivers/display/spi/

include $(SDIR)/target/drivers/mcu/emu/Makefile.inc
include $(SDIR)/Makefile.inc