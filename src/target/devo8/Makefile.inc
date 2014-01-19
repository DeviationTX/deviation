SCREENSIZE := 320x240x16
FILESYSTEM := devo8
DFU_ARGS   := -c 8 -b 0x08004000

include $(SDIR)/target/common/devo/Makefile.inc
