SCREENSIZE  := text
FILESYSTEMS := common text
LANGUAGE    := devo10 

CRC_OFFSET       := 8192
OPTIMIZE_DFU     := 1
MODULAR          := 0x20004000
DFU_ARGS         := -c 7 -b 0x08003000
# 0x08003000

#PRE_FS = $(ODIR)/.pre_fs

#INCLUDE_FS := 0

include target/common/devo/Makefile.inc
#include target/common/devo/Makefile.devofs.inc

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
	rm filesystem/$(FILESYSTEM)/datalog.bin
endif
