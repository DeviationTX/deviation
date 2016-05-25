SCREENSIZE  := 320x240x16
FILESYSTEMS := common base_fonts 320x240x16
FONTS        = filesystem/$(FILESYSTEM)/media/15normal.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon
LANGUAGE    := devo8

include target/common/emu/Makefile.inc
