FILESYSTEM := devo8
SCREENSIZE := 320x240x16
DFU_ARGS   := -c 8 -b 0x08004000

include target/common_devo/Makefile.inc

SRC_C   += $(wildcard target/$(TARGET)/*.c) \
           target/devo8/backlight.c \
           target/devo8/lcd.c \
           target/devo8/tx_voltage.c

