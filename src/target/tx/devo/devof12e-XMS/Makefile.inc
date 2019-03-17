FILESYSTEMS := common text text_gfx
SCREENSIZE  := text
DISPLAY_DRIVER := i2c/video_tw8816.c
DFU_ARGS    := -c 12 -b 0x08004000
LANGUAGE    := devo10
FONTS        = filesystem/$(FILESYSTEM)/media/f12e.fon

OPTIMIZE_DFU := 1

include $(SDIR)/target/tx/devo/common/Makefile.inc

ifndef BUILD_TARGET

SRC_C   += $(SDIR)/target/tx/devo/devof12e/crc.c \
           $(SDIR)/target/tx/devo/devof12e/lcd.c \
           $(SDIR)/target/tx/devo/devof12e/mapped_gfx.c

else

$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/=12normal/=f12e/' filesystem/$(FILESYSTEM)/media/config.ini

endif
