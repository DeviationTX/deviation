SCREENSIZE  := 480x272x16
FILESYSTEMS := common base_fonts 320x240x16 480x272x16
DFU_ARGS    := -c 12 -b 0x08004000
FONTS        = filesystem/$(FILESYSTEM)/media/15normal.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon
LANGUAGE    := devo8,devo12

include $(SDIR)/target/common/devo/Makefile.inc

ifdef BUILD_TARGET

$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/  drawn_background=0/  drawn_background=1/' filesystem/$(FILESYSTEM)/media/config.ini
	rm -f filesystem/$(FILESYSTEM)/media/config.ini.bak

endif
