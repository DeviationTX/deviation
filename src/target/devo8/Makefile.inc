CROSS    = arm-none-eabi-

SRC_C    = $(wildcard target/$(TARGET)/*.c)

SCREENSIZE = qvga
LINKFILE = target/$(TARGET)/$(TARGET).ld

CFLAGS   = -DSTM32F1 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd
LFLAGS   = -nostartfiles --gc-sections -nostartfiles -Wl,-T$(LINKFILE) -lopencm3_stm32f1 -lc -lnosys

all = $(TARGET).dfu
$(TARGET).dfu: $(TARGET).bin
	../utils/dfu.py -b 0x08004000:$< $@

$(TARGET).bin: $(TARGET).elf
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(TARGET).list
