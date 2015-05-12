SCREENSIZE := text
FILESYSTEM := devof7

DFU_ARGS         := -c 12 -b 0x08004000
# 0x08003000

include $(SDIR)/target/common/devo/Makefile.inc

