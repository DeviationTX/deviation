FILESYSTEMS := common text
SCREENSIZE  := text

CRC_OFFSET       := 8192
OPTIMIZE_DFU     := 1
MODULAR          := 0x20004000
DFU_ARGS         := -c 7 -b 0x08003000

include target/common/devo/Makefile.inc

ifndef BUILD_TARGET

SRC_C   += $(SDIR)/target/devof7/backlight.c \
           $(SDIR)/target/devof7/channels.c \
           $(SDIR)/target/devof7/crc.c \
           $(SDIR)/target/devof7/ia9211_map.c \
           $(SDIR)/target/devof7/lcd.c \
           $(SDIR)/target/devof7/tx_buttons.c

endif
