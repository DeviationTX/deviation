FILESYSTEMS := common text
SCREENSIZE  := text
DISPLAY_DRIVER := spi/video_ia9211.c

OPTIMIZE_DFU     := 1
MODULAR          := 0x20004000
DFU_ARGS         := -c 7 -b 0x08003000

SRC_C   += $(SDIR)/target/tx/devo/devof7/channels.c \
           $(SDIR)/target/tx/devo/devof7/crc.c

TARGET=devof7-XMS

include $(SDIR)/target/tx/devo/common/Makefile.inc
include $(SDIR)/Makefile.inc