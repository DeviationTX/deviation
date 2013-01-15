SCREENSIZE = 320x240x16
FILESYSTEM = devo8
include target/common_devo/Makefile.inc

$(TARGET).dfu: $(TARGET).bin
	../utils/dfu.py -c 8 -b 0x08004000:$< $@
	../utils/get_mem_usage.pl $(TARGET).map
