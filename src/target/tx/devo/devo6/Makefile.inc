FILESYSTEMS := common base_fonts 320x240x16
SCREENSIZE  := 320x240x16
DISPLAY_DRIVER := 8080/320x240x16.c
DFU_ARGS    := -c 8 -b 0x08004000
FONTS        = filesystem/$(FILESYSTEM)/media/15normal.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon
LANGUAGE    := devo8

include $(SDIR)/target/tx/devo/common/Makefile.inc

ifndef BUILD_TARGET
endif
