SCREENSIZE := 128x64x1
FILESYSTEM := devo7e

CRC_OFFSET       := 8192
DFU_ARGS         := -c 7 -b 0x08003000

PRE_FS = $(ODIR)/.pre_fs

include $(SDIR)/target/common/devo/Makefile.inc

ifndef BUILD_TARGET

SRC_C   += $(SDIR)/target/devo7e/backlight.c \
					 $(SDIR)/target/devo7e/crc.c \
           $(SDIR)/target/devo7e/lcd.c

else

$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/; has_pa-cyrf6936 = 1/  has_pa-cyrf6936 = 0/' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/;switch_types: 3x2, 3x1, 2x2/;switch_types: 3x4, 3x3, 3x2, 3x1, 2x2, 2x1, potx2, potx1\n;May occur more than once if necessary./' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/;extra-switches=/; extra-switches = 3x4\n; extra-switches = 2x2\n; extra-switches = potx2/' filesystem/$(FILESYSTEM)/hardware.ini

$(PRE_FS):
	mkdir -p filesystem/$(FILESYSTEM)/media/ 2>/dev/null || true
	cp -prf filesystem/devo10/* filesystem/$(FILESYSTEM)/
	rm -rf filesystem/$(FILESYSTEM)/language 2>/dev/null || true
	touch $(PRE_FS)

endif
