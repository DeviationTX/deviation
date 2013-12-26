SCREENSIZE := 480x272x16
FILESYSTEM := devo12

PRE_FS = $(ODIR)/.pre_fs

include target/common/emu/Makefile.inc

ifdef BUILD_TARGET

$(PRE_FS):
	mkdir -p filesystem/$(FILESYSTEM)/media/ 2>/dev/null || true
	cp -prf filesystem/devo8/* filesystem/$(FILESYSTEM)/
	cp -prf filesystem/common/media filesystem/$(FILESYSTEM)/
	rm -rf filesystem/$(FILESYSTEM)/language 2>/dev/null || true
	touch $(PRE_FS)
endif
