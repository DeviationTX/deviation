SCREENSIZE := text
FILESYSTEM := devof7
NUM_MODELS ?= 10

PRE_FS = $(ODIR)/.pre_fs

include target/common/emu/Makefile.inc

ifdef BUILD_TARGET
$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/; has_pa-cyrf6936 = 1/  has_pa-cyrf6936 = 0/' filesystem/$(FILESYSTEM)/tx.ini

#$(PRE_FS):
#	mkdir -p filesystem/$(FILESYSTEM)/media/ 2>/dev/null || true
#	cp -prf filesystem/devo10/* filesystem/$(FILESYSTEM)/
#	rm -rf filesystem/$(FILESYSTEM)/language 2>/dev/null || true
#	touch $(PRE_FS)
endif
