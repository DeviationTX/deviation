SCREENSIZE  := text
FILESYSTEMS := common text text_gfx
LANGUAGE    := devo10
FONTS        = filesystem/$(FILESYSTEM)/media/f12e.fon

include target/drivers/mcu/emu/Makefile.inc

ifdef BUILD_TARGET

$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/=12normal/=f12e/' filesystem/$(FILESYSTEM)/media/config.ini

endif
