SCREENSIZE := text
FILESYSTEM := devof7

DFU_ARGS         := -c 12 -b 0x08004000

DEFAULT_PROTOCOLS := 
include $(SDIR)/target/common/devo/Makefile.inc
include target/common/devo/Makefile.devofs.inc

ifndef BUILD_TARGET

MEDIA_FILES = 
NUM_MODELS ?= 10

else

#$(TARGET)-lib.dfu: $(wildcard filesystem/devof7/media/*.*) $(TARGET).dfu $(PRE_FS) $(TARGET).fs_wrapper
#	target/common/devo/devofs/buildfs.pl -invert filesystem/devof7 > $(ODIR)/$(TARGET)-lib.bin
#	dd if=$(ODIR)/$(TARGET)-lib.bin of=$(ODIR)/$(TARGET)-lib.1.bin bs=4096 count=1
#	dd if=$(ODIR)/$(TARGET)-lib.bin of=$(ODIR)/$(TARGET)-lib.2.bin bs=4096 skip=1
#	../utils/dfu.py --name "$(HGVERSION) Library" --alt 2 -b 0x0000:$(ODIR)/$(TARGET)-lib.1.bin --alt 1 -b 0x1000:$(ODIR)/$(TARGET)-lib.2.bin $@

$(TARGET).fs_wrapper: $(LAST_MODEL)
	rm filesystem/devof7/media/*.fon
	rm filesystem/devof7/datalog.bin
endif
