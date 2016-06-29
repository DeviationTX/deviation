SCREENSIZE := 128x64x1
FILESYSTEMS := common base_fonts 128x64x1
FONTS        = filesystem/$(FILESYSTEM)/media/12normal.fon \
               filesystem/$(FILESYSTEM)/media/04b03.fon

include target/common/emu/Makefile.inc

ifdef BUILD_TARGET
$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/; has_pa-cyrf6936 = 1/  has_pa-cyrf6936 = 0/' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/;switch_types: 3x2, 3x1, 2x2/;switch_types: 3x4, 3x3, 3x2, 3x1, 2x8, 2x7, 2x6, 2x5, 2x4, 2x3, 2x2, 2x1, potx2, potx1\n;May occur more than once if necessary.\n;Add nostock if stock FMOD and HOLD switches have been removed./' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/;extra-switches=/; extra-switches = nostock\n; extra-switches = 3x4\n; extra-switches = 2x2\n; extra-switches = potx2/' filesystem/$(FILESYSTEM)/hardware.ini
endif
