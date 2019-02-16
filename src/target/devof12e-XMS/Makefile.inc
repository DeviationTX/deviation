FILESYSTEMS := common text text_gfx
SCREENSIZE  := text
DISPLAY_DRIVER := i2c/video_tw8816.c
DFU_ARGS    := -c 12 -b 0x08004000
LANGUAGE    := devo10
FONTS        = filesystem/$(FILESYSTEM)/media/18bold.fon

OPTIMIZE_DFU := 1

include $(SDIR)/target/common/devo/Makefile.inc

ifndef BUILD_TARGET

SRC_C   += $(SDIR)/target/devof12e/backlight.c \
           $(SDIR)/target/devof12e/channels.c \
           $(SDIR)/target/devof12e/crc.c \
           $(SDIR)/target/devof12e/lcd.c \
           $(SDIR)/target/devof12e/mapped_gfx.c \
           $(SDIR)/target/devof12e/tx_buttons.c
endif
