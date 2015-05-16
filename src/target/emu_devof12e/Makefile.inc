SCREENSIZE := text
FILESYSTEM := devof7
NUM_MODELS ?= 10

PRE_FS = $(ODIR)/.pre_fs

include target/common/emu/Makefile.inc
