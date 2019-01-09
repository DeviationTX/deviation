FILESYSTEMS := common base_fonts 320x240x16

SCREENSIZE  := 320x240x16
FONTS        = filesystem/$(FILESYSTEM)/media/15ascii.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon

ifndef BUILD_TARGET
CROSS    = arm-none-eabi-

ALL = $(LIBOPENCM3) $(TARGET).bin

NUM_MODELS ?= 10

LINKFILE = $(SDIR)/target/$(TARGET)/$(TARGET).ld
LIBOPENCM3 = $(SDIR)/libopencm3/lib/libopencm3_stm32f1.a

SRC_C   := $(wildcard $(SDIR)/target/$(TARGET)/*.c) \
           $(wildcard $(SDIR)/target/common/stm32/*.c) \
           $(wildcard $(SDIR)/target/common/filesystems/*.c) \
           $(wildcard $(SDIR)/target/common/filesystems/devofs/*.c) \
           $(wildcard $(SDIR)/target/common/filesystems/petit_fat/*.c) \
           $(wildcard $(SDIR)/target/common/devo/msc2/*.c) \
           $(wildcard $(SDIR)/target/common/devo/msc2/lib/*.c) \
           $(wildcard $(SDIR)/target/common/devo/hid/*.c) \
           $(wildcard $(SDIR)/target/common/devo/protocol/*.c) \
           $(wildcard $(SDIR)/target/common/devo/uart.c)

SRC_C := $(filter-out $(SDIR)/target/common/stm32/spi_flash.c, $(SRC_C))

CFLAGS   = -D"assert_param(x)=" -DSTM32F10X_HD -DSTM32F1 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fdata-sections -ffunction-sections -I$(SDIR)/target/common/devo/msc2/lib -I$(SDIR)/target/common/devo/msc2 -I$(SDIR)/libopencm3/include -I$(SDIR)/target/common/filesystems -fno-builtin-printf -Os --specs=nano.specs
MODULE_FLAGS = -fno-builtin

LFLAGS   = -nostartfiles -Wl,-gc-sections -Wl,-Map=$(TARGET).map,--cref -lc -lnosys -L$(SDIR) -Lobjs/$(TARGET)
LFLAGS2  = -Wl,-T$(LINKFILE)


else #BUILD_TARGET

$(TARGET).bin: $(TARGET).elf
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(TARGET).list

$(LIBOPENCM3):
	+$(MAKE) -C $(SDIR)/libopencm3 TARGETS=stm32/f1 lib


endif #BUILD_TARGET
