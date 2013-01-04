CROSS    = arm-none-eabi-
EXTRA_MAKEFILES := $(EXTRA_MAKEFILES) target/common_devo/Makefile.inc

LINKFILE = target/$(TARGET)/$(TARGET).ld
LIBOPENCM3 = libopencm3/lib/libopencm3_stm32f1.a

CFLAGS   = -D"assert_param(x)=" -DSTM32F10X_HD -DSTM32F1 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fdata-sections -ffunction-sections -Itarget/common_devo/msc2/lib -Itarget/common_devo/msc2 -Ilibopencm3/include -fno-builtin-printf -Os
LFLAGS   = -nostartfiles -Wl,-gc-sections -Wl,-T$(LINKFILE) -Wl,-Map=$(TARGET).map,--cref -lc 
#-lnosys

all: $(TARGET).dfu

$(TARGET).bin: $(TARGET).elf
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(TARGET).list

$(LIBOPENCM3):
	make -C libopencm3 TARGETS=stm32/f1 lib
