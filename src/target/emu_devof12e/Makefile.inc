SCREENSIZE  := text
NUM_MODELS  ?= 10
FILESYSTEMS := common text_gfx

PRE_FS = $(ODIR)/.pre_fs

include target/common/emu/Makefile.inc
