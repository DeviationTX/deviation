SCREENSIZE := 128x64x1
FILESYSTEM := devo7e

CRC_OFFSET       := 8192
OPTIMIZE_DFU     := 1
MODULAR          := 0x20004000
DFU_ARGS         := -c 7 -b 0x08003000

PRE_FS = $(ODIR)/.pre_fs

include $(SDIR)/target/common/devo/Makefile.inc

ifdef BUILD_TARGET
$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/; has_pa-cyrf6936 = 1/  has_pa-cyrf6936 = 0/' filesystem/$(FILESYSTEM)/hardware.ini

$(PRE_FS):
	mkdir -p filesystem/$(FILESYSTEM)/media/ 2>/dev/null || true
	cp -prf filesystem/devo10/* filesystem/$(FILESYSTEM)/
	rm -rf filesystem/$(FILESYSTEM)/language 2>/dev/null || true
	touch $(PRE_FS)

endif
