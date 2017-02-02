SCREENSIZE  := 480x272x16
FILESYSTEMS := common base_fonts 320x240x16 480x272x16
FONTS        = filesystem/$(FILESYSTEM)/media/15normal.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon
LANGUAGE    := devo8,devo12

include target/common/emu/Makefile.inc
