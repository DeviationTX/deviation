SCREENSIZE  := 320x240x16
FILESYSTEMS := common base_fonts 320x240x1
FONTS        = filesystem/$(FILESYSTEM)/media/15ascii.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon

NUM_MODELS ?= 10

include target/drivers/mcu/emu/Makefile.inc

ifdef BUILD_TARGET
$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/=15normal/=15ascii/' filesystem/$(FILESYSTEM)/media/config.ini
	rm filesystem/$(FILESYSTEM)/datalog.bin
endif
