SCREENSIZE := 320x240x16
FILESYSTEM := devo8
DFU_ARGS   := -c 12 -b 0x08004000

include target/common_devo/Makefile.inc

ifndef BUILD_TARGET
ALL += $(TARGET)-lib.dfu
MEDIA_FILES = 
else
$(TARGET)-lib.dfu: $(wildcard filesystem/devo8/media/*.*) $(TARGET).dfu
	/bin/rm -rf $(ODIR)/filesystem 2> /dev/null; true
	/bin/mkdir $(ODIR)/filesystem
	/bin/cp -prf filesystem/devo8/media $(ODIR)/filesystem/
	../utils/mkfat/mkfat.py 0 $(ODIR)/filesystem/ $(ODIR)/$(TARGET)-lib.bin
	../utils/dfu.py --alt 2 --name "$(HGVERSION) Library" -b 0x64080000:$(ODIR)/$(TARGET)-lib.bin $@

$(TARGET).fs:
	/bin/rm -rf $(ODIR)/filesystem; /bin/cp -prf filesystem/devo8 $(ODIR)/filesystem
	rm -f deviation-fs-$(HGVERSION).zip
	zip -r deviation-fs-$(HGVERSION).zip $(TARGET)-lib.dfu
	export p=`pwd`; cd $(ODIR); zip -x filesystem/media/\* -ur $$p/deviation-fs-$(HGVERSION).zip filesystem

endif #BUILD_TARGET
