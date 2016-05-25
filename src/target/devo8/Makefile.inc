FILESYSTEMS := common base_fonts 320x240x16
SCREENSIZE  := 320x240x16
DFU_ARGS    := -c 8 -b 0x08004000
FONTS        = filesystem/$(FILESYSTEM)/media/15normal.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon
LANGUAGE    := devo8

include $(SDIR)/target/common/devo/Makefile.inc
