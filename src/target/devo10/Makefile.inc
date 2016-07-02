FILESYSTEMS := common base_fonts 128x64x1
SCREENSIZE  := 128x64x1
DFU_ARGS    := -c 10 -b 0x08004000
FONTS        = filesystem/$(FILESYSTEM)/media/12normal.fon \
               filesystem/$(FILESYSTEM)/media/04b03.fon
LANGUAGE   := devo10

include $(SDIR)/target/common/devo/Makefile.inc
