SCREENSIZE  := 128x64x1
FILESYSTEMS := common base_fonts 128x64x1
FONTS        = filesystem/$(FILESYSTEM)/media/12normal.fon \
               filesystem/$(FILESYSTEM)/media/04b03.fon
LANGUAGE    := devo10

CRC_OFFSET       := 8192
DFU_ARGS         := -c 7 -b 0x08003000

include $(SDIR)/target/common/devo/Makefile.inc

ifdef BUILD_TARGET

$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/; enable-a7105    = A13/  enable-a7105    = A13/' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/; has_pa-a7105    = 1/  has_pa-a7105    = 1/' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/; enable-cc2500   = A14/  enable-cc2500   = A14/' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/; has_pa-cc2500   = 1/  has_pa-cc2500   = 1/' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/; enable-nrf24l01 = A14/  enable-nrf24l01 = A15/' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/; has_pa-nrf24l01 = 1/  has_pa-nrf24l01 = 1/' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/;switch_types: 3x2, 3x1, 2x2/;switch_types: 3x4, 3x3, 3x2, 3x1, 2x8, 2x7, 2x6, 2x5, 2x4, 2x3, 2x2, 2x1, potx2, potx1\n;May occur more than once if necessary.\n;Add nostock if stock FMOD and HOLD switches have been removed./' filesystem/$(FILESYSTEM)/hardware.ini
	perl -p -i -e 's/;extra-switches=/  extra-switches = nostock\n  extra-switches = 3x4\n; extra-switches = 2x2\n  extra-switches = potx2/' filesystem/$(FILESYSTEM)/hardware.ini
	rm -f filesystem/$(FILESYSTEM)/hardware.ini.bak

endif
