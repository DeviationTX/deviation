SCREENSIZE  := 480x272x16
FILESYSTEMS := common base_fonts 320x240x16 480x272x16
DFU_ARGS    := -c 12 -b 0x08004000
FONTS        = filesystem/$(FILESYSTEM)/media/15normal.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon
LANGUAGE    := devo8,devo12

include $(SDIR)/target/common/devo/Makefile.inc

ifndef BUILD_TARGET
ALL += $(TARGET)-lib.dfu
MEDIA_FILES = 
else
$(TARGET)-lib.dfu: $(LAST_MODEL) $(TARGET).dfu $(PRE_FS)
	/bin/rm -rf $(ODIR)/filesystem 2> /dev/null; true
	/bin/mkdir $(ODIR)/filesystem
	/bin/cp -prf filesystem/devo12/media $(ODIR)/filesystem/
	../utils/mkfat/mkfat.py --size 16M $(ODIR)/filesystem/ $(ODIR)/$(TARGET)-lib.bin
	../utils/dfu.py --alt 2 --name "$(HGVERSION) Library" -b 0x64080000:$(ODIR)/$(TARGET)-lib.bin $@

$(TARGET).fs:
	/bin/rm -rf $(ODIR)/filesystem; /bin/cp -prf filesystem/devo12 $(ODIR)/filesystem
	rm -f deviation-fs-$(HGVERSION).zip
	zip -r deviation-fs-$(HGVERSION).zip $(TARGET)-lib.dfu
	export p=`pwd`; cd $(ODIR); zip -ur $$p/deviation-fs-$(HGVERSION).zip filesystem -x filesystem/media/\*

$(TARGET).zip: $(ALL)
	cp -f $(TARGET).dfu deviation-$(HGVERSION).dfu
	rm -f deviation-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	zip debug-$(HGVERSION).zip $(TARGET).elf
	# make zipping more compatible with other distributions
	zip deviation-$(HGVERSION).zip deviation-$(HGVERSION).dfu
ifeq "$(INCLUDE_FS)" "1"
	zip -u deviation-$(HGVERSION).zip $(TARGET)-lib.dfu
	export p=`pwd`; cd filesystem/$(FILESYSTEM) && zip -ur $$p/deviation-$(HGVERSION).zip  * -x media/\*
endif
	zip -u deviation-$(HGVERSION).zip debug-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	rm -f deviation-$(HGVERSION).dfu

endif #BUILD_TARGET
