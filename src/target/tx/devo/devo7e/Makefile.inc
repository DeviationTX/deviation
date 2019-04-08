SCREENSIZE  := 128x64x1
DISPLAY_DRIVER := spi/128x64x1.c
FILESYSTEMS := common base_fonts 128x64x1
ifdef NEW_BOOTLOADER
    DFU_ARGS    := -c 0 -b 0x08003000
    DFU_STRING   = "$(HGVERSION) Unified Firmware"
else
    DFU_ARGS    := -c 7 -b 0x08003000
endif

FONTS        = filesystem/$(FILESYSTEM)/media/12normal.fon \
               filesystem/$(FILESYSTEM)/media/04b03.fon
LANGUAGE     = devo10

OPTIMIZE_DFU     := 1
MODULAR          := 0x20004000
ENABLE_LTO       := 1

include $(SDIR)/target/tx/devo/common/Makefile.inc

ifdef BUILD_TARGET
$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/; has_pa-cyrf6936 = 1/  has_pa-cyrf6936 = 0/' filesystem/$(FILESYSTEM)/hardware.ini

endif
