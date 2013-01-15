SCREENSIZE := 128x64x1
FILESYSTEM=devo10

CRC_OFFSET := 8192
OPTIMIZE_DFU := 1
#NO_STANDARD_GUI := 1
MODULAR=0x20004000

include target/common_devo/Makefile.inc


$(TARGET).dfu: $(TARGET).bin
	../utils/dfu.py -c 7 -b 0x08003000:$< $@
	../utils/get_mem_usage.pl $(TARGET).map
