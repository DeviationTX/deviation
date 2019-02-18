SCREENSIZE  := text
FILESYSTEMS := common text text_gfx
LANGUAGE    := devo10
FONTS        = filesystem/$(FILESYSTEM)/media/18bold.fon

include target/drivers/mcu/emu/Makefile.inc

