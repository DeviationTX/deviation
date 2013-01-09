SCREENSIZE := 128x64x1
CRC_OFFSET := 8192
OPTIMIZE_DFU := 1
include target/common_devo/Makefile.inc

SRC_C    = $(wildcard target/$(TARGET)/*.c) $(wildcard target/common_devo/*.c) $(wildcard target/common_devo/msc2/*.c) $(wildcard target/common_devo/msc2/lib/*.c)


$(TARGET).dfu: $(TARGET).bin
	../utils/dfu.py -c 7 -b 0x08003000:$< $@
	../utils/get_mem_usage.pl $(TARGET).map
