FILESYSTEM = devo8
SCREENSIZE = 320x240x16

include target/common_devo/Makefile.inc

SRC_C    = $(wildcard target/$(TARGET)/*.c) \
           target/devo8/backlight.c \
           target/devo8/lcd.c \
           target/devo8/tx_voltage.c


$(TARGET).dfu: $(TARGET).bin
	../utils/dfu.py -c 8 -b 0x08004000:$< $@
	../utils/get_mem_usage.pl $(TARGET).map
