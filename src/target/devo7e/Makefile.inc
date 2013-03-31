SCREENSIZE := 128x64x1
FILESYSTEM := devo7e

CRC_OFFSET       := 8192
OPTIMIZE_DFU     := 1
#NO_STANDARD_GUI := 1
MODULAR          := 0x20004000
DFU_ARGS         := -c 7 -b 0x08003000

include target/common_devo/Makefile.inc

ifdef BUILD_TARGET
$(TARGET).fs_wrapper: $(LAST_MODEL)
	cp -pf filesystem/devo10/media/*.fon filesystem/$(FILESYSTEM)/media/
	rm -rf filesystem/$(FILESYSTEM)/language
	perl -p -i -e 's/; has_pa-cyrf6936 = 1/  has_pa-cyrf6936 = 0/' filesystem/$(FILESYSTEM)/tx.ini
endif
