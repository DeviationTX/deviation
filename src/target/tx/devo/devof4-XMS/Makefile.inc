FILESYSTEMS := common text
SCREENSIZE  := text

OPTIMIZE_DFU     := 1
MODULAR          := 0x20004000
DFU_ARGS         := -c 4 -b 0x08003000

include target/tx/devo/common/Makefile.inc

ifndef BUILD_TARGET

SRC_C   += $(SDIR)/target/tx/devo/devof7/backlight.c\
           $(SDIR)/target/tx/devo/devof7/ia9211_map.c \
           $(SDIR)/target/tx/devo/devof7/lcd.c \
           $(SDIR)/target/tx/devo/devof7/tx_buttons.c \
           $(SDIR)/target/tx/devo/devof7/crc.c

endif
