SCREENSIZE  := 128x64x1
FILESYSTEMS := common base_fonts 128x64x1
FONTS        = filesystem/$(FILESYSTEM)/media/12normal.fon \
               filesystem/$(FILESYSTEM)/media/04b03.fon
LANGUAGE     = devo10

TARGET = emu_devo7e
FAMILY = devo

include $(SDIR)/target/drivers/mcu/emu/Makefile.inc
include $(SDIR)/Makefile.inc

$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/; has_pa-cyrf6936 = 1/  has_pa-cyrf6936 = 0/' filesystem/$(FILESYSTEM)/hardware.ini
