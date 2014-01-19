SCREENSIZE := 480x272x16
FILESYSTEM := devo12
DFU_ARGS   := -c 12 -b 0x08004000

PRE_FS = $(ODIR)/.pre_fs

include $(SDIR)/target/common/devo/Makefile.inc

ifndef BUILD_TARGET
ALL += $(TARGET)-lib.dfu
MEDIA_FILES = 
else
$(TARGET)-lib.dfu: $(wildcard filesystem/devo12/media/*.*) $(TARGET).dfu $(PRE_FS)
	/bin/rm -rf $(ODIR)/filesystem 2> /dev/null; true
	/bin/mkdir $(ODIR)/filesystem
	/bin/cp -prf filesystem/devo12/media $(ODIR)/filesystem/
	../utils/mkfat/mkfat.py --size 16M $(ODIR)/filesystem/ $(ODIR)/$(TARGET)-lib.bin
	../utils/dfu.py --alt 2 --name "$(HGVERSION) Library" -b 0x64080000:$(ODIR)/$(TARGET)-lib.bin $@

$(PRE_FS):
	mkdir -p filesystem/$(FILESYSTEM)/media/ 2>/dev/null || true
	cp -prf filesystem/devo8/* filesystem/$(FILESYSTEM)/
	cp -prf filesystem/common/media filesystem/$(FILESYSTEM)/
	rm -rf filesystem/$(FILESYSTEM)/language 2>/dev/null || true
	touch $(PRE_FS)

$(TARGET).fs:
	/bin/rm -rf $(ODIR)/filesystem; /bin/cp -prf filesystem/devo12 $(ODIR)/filesystem
	rm -f deviation-fs-$(HGVERSION).zip
	zip -r deviation-fs-$(HGVERSION).zip $(TARGET)-lib.dfu
	export p=`pwd`; cd $(ODIR); zip -x filesystem/media/\* -ur $$p/deviation-fs-$(HGVERSION).zip filesystem

$(TARGET).zip: $(TARGET).dfu $(ALL)
	mv -f $(TARGET).dfu deviation-$(HGVERSION).dfu
	rm -f deviation-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	zip debug-$(HGVERSION).zip $(TARGET).elf
	# make zipping more compatible with other distributions
	zip deviation-$(HGVERSION).zip deviation-$(HGVERSION).dfu
ifeq "$(INCLUDE_FS)" "1"
	zip -u deviation-$(HGVERSION).zip $(TARGET)-lib.dfu
	export p=`pwd`; cd filesystem/$(FILESYSTEM) && zip -x media/\* -ur $$p/deviation-$(HGVERSION).zip  *
endif
	zip -u deviation-$(HGVERSION).zip debug-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	rm -f deviation-$(HGVERSION).dfu

endif #BUILD_TARGET
