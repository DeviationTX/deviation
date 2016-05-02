FILESYSTEM := devo8
SCREENSIZE  := 320x240x16
DFU_ARGS   := -D 0x0483:0xdf12 -b 0x08003000

ifndef BUILD_TARGET
CROSS    = arm-none-eabi-

LINKFILE = $(SDIR)/target/$(TARGET)/$(TARGET).ld
LIBOPENCM3 = $(SDIR)/libopencm3/lib/libopencm3_stm32f1.a

SRC_C    = $(wildcard $(SDIR)/target/$(TARGET)/*.c) \
           $(wildcard $(SDIR)/target/common/stm32/*.c) \
           $(wildcard $(SDIR)/target/common/devo/devofs/*.c) \
           $(wildcard $(SDIR)/target/common/devo/petit_fat/*.c) \
           $(wildcard $(SDIR)/target/common/devo/msc2/*.c) \
           $(wildcard $(SDIR)/target/common/devo/msc2/lib/*.c) \
           $(wildcard $(SDIR)/target/common/devo/hid/*.c) \
           $(wildcard $(SDIR)/target/common/devo/protocol/*.c) \
           $(SDIR)/target/devo8/ili9341.c

CFLAGS   = -D"assert_param(x)=" -DSTM32F10X_HD -DSTM32F1 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fdata-sections -ffunction-sections -I$(SDIR)/target/common/devo/msc2/lib -I$(SDIR)/target/common/devo/msc2 -I$(SDIR)/libopencm3/include -I$(SDIR)/target/common/devo/devofs -fno-builtin-printf -Os --specs=nano.specs
MODULE_FLAGS = -fno-builtin

LFLAGS   = -nostartfiles -Wl,-gc-sections -Wl,-Map=$(TARGET).map,--cref -lc -lnosys -L$(SDIR)
LFLAGS2  = -Wl,-T$(LINKFILE)

ALL = $(LIBOPENCM3) $(TARGET).dfu

else #BUILD_TARGET

$(TARGET).bin: $(TARGET).elf
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(TARGET).list

$(LIBOPENCM3):
	$(MAKE) -C $(SDIR)/libopencm3 TARGETS=stm32/f1 lib

$(TARGET).dfu: $(TARGET).bin
	$(SDIR)/../utils/dfu.py --name "$(HGVERSION) Firmware" $(DFU_ARGS):$< $@
	$(SDIR)/../utils/get_mem_usage.pl $(TARGET).map

endif #BUILD_TARGET
