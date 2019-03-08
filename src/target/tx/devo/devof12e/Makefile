TARGET=devof12e
SCREENSIZE  := text
DISPLAY_DRIVER := i2c/video_tw8816.c
FILESYSTEMS := common text text_gfx

DFU_ARGS         := -c 12 -b 0x08004000

DEFAULT_PROTOCOLS :=
INCLUDE_FS := 0
OPTIMIZE_DFU := 1

MEDIA_FILES =
NUM_MODELS ?= 10

include $(SDIR)/target/tx/devo/common/Makefile.inc
include $(SDIR)/Makefile.inc

$(TARGET).fs_wrapper: $(LAST_MODEL)
	rm filesystem/$(FILESYSTEM)/datalog.bin
