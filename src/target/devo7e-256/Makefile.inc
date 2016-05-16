SCREENSIZE  := 128x64x1
FILESYSTEMS := common base_fonts 128x64x1
FONTS        = filesystem/$(FILESYSTEM)/media/12normal.fon \
               filesystem/$(FILESYSTEM)/media/04b03.fon
LANGUAGE    := devo10

CRC_OFFSET       := 8192
DFU_ARGS         := -c 7 -b 0x08003000

include $(SDIR)/target/common/devo/Makefile.inc

ifndef BUILD_TARGET

SRC_C   += $(SDIR)/target/devo7e/backlight.c \
					 $(SDIR)/target/devo7e/channels.c \
					 $(SDIR)/target/devo7e/crc.c \
           $(SDIR)/target/devo7e/lcd.c \
           $(SDIR)/target/devo7e/tx_buttons.c

else

$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/; has_pa-cyrf6936 = 1/  has_pa-cyrf6936 = 0/' filesystem/$(FILESYSTEM)/hardware.ini

endif
