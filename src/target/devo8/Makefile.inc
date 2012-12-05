CROSS    = arm-none-eabi-

SRC_C    = $(wildcard target/$(TARGET)/*.c) $(wildcard target/common_devo/*.c) $(wildcard target/common_devo/msc2/*.c) $(wildcard target/common_devo/msc2/lib/*.c)

SCREENSIZE = 320x240x16
LINKFILE = target/$(TARGET)/$(TARGET).ld

CFLAGS   = -D"assert_param(x)=" -DSTM32F10X_HD -DSTM32F1 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fdata-sections -ffunction-sections -Itarget/common_devo/msc2/lib -Itarget/common_devo/msc2 -fno-builtin-printf -Os
LFLAGS   = -nostartfiles -Wl,-gc-sections -Wl,-T$(LINKFILE) -Wl,-Map=$(TARGET).map,--cref -lopencm3_stm32f1 -lc 
#-lnosys

all = $(TARGET).dfu
$(TARGET).dfu: $(TARGET).bin
	../utils/dfu.py -c 8 -b 0x08004000:$< $@
	../utils/get_mem_usage.pl $(TARGET).map

$(TARGET).bin: $(TARGET).elf
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(TARGET).list
