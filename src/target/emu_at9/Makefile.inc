SCREENSIZE  := 320x240x16
FILESYSTEMS := common base_fonts 320x240x1
FONTS        = filesystem/$(FILESYSTEM)/media/23bold.fon

include target/common/emu/Makefile.inc
