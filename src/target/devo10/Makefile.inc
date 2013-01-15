FILESYSTEM=devo10
SCREENSIZE = 128x64x1

include target/common_devo/Makefile.inc

$(TARGET).dfu: $(TARGET).bin
	../utils/dfu.py -c 10 -b 0x08004000:$< $@
	../utils/get_mem_usage.pl $(TARGET).map
