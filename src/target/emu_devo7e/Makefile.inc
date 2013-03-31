SCREENSIZE := 128x64x1
FILESYSTEM := devo7e

include target/common_emu/Makefile.inc

ifdef BUILD_TARGET
$(TARGET).fs_wrapper: $(LAST_MODEL)
	cp -pf filesystem/devo10/media/*.fon filesystem/$(FILESYSTEM)/media/
	rm -rf filesystem/$(FILESYSTEM)/language
	perl -p -i -e 's/; has_pa-cyrf6936 = 1/  has_pa-cyrf6936 = 0/' filesystem/$(FILESYSTEM)/tx.ini
endif
