include target/common_devo/Makefile.inc

SRC_C    = $(wildcard target/$(TARGET)/*.c) $(wildcard target/common_devo/*.c) $(wildcard target/common_devo/msc2/*.c) $(wildcard target/common_devo/msc2/lib/*.c)

SCREENSIZE = 128x64x1

$(TARGET).dfu: $(TARGET).bin
	../utils/dfu.py -c 10 -b 0x08004000:$< $@
	../utils/get_mem_usage.pl $(TARGET).map
