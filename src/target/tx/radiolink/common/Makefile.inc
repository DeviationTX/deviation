ifndef BUILD_TARGET
CROSS    = arm-none-eabi-

LINKFILE = $(SDIR)/target/tx/$(FAMILY)/$(TARGET)/$(TARGET).ld
LIBOPENCM3 = $(SDIR)/libopencm3/lib/libopencm3_stm32f1.a

ifeq "$(HAS_4IN1_FLASH)" "1"
NUM_MODELS ?= 30
else
NUM_MODELS ?= 10
endif

SRC_C   := $(wildcard $(SDIR)/target/tx/$(FAMILY)/common/*.c) \
           $(wildcard $(SDIR)/target/tx/$(FAMILY)/$(TARGET)/*.c) \
           $(wildcard $(SDIR)/target/drivers/mcu/stm32/*.c) \
           $(wildcard $(SDIR)/target/drivers/indicators/*.c) \
           $(wildcard $(SDIR)/target/drivers/filesystems/*.c) \
           $(wildcard $(SDIR)/target/drivers/filesystems/devofs/*.c) \
           $(wildcard $(SDIR)/target/drivers/filesystems/petit_fat/*.c) \
           $(wildcard $(SDIR)/target/tx/devo/common/protocol/*.c) \
           $(wildcard $(SDIR)/target/drivers/serial/uart/*.c) \
           $(wildcard $(SDIR)/target/drivers/usb/*.c) \
           $(SDIR)/target/drivers/storage/mcu_flash.c \
           $(SDIR)/target/drivers/backlight/backlight.c \
           $(SDIR)/target/drivers/input/analog/analog.c

ifeq "$(HAS_4IN1_FLASH)" "1"
SRC_C   += $(SDIR)/target/drivers/storage/spi_flash.c
endif

CFLAGS   = -D"assert_param(x)=" -DSTM32F10X_HD -DSTM32F1 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fdata-sections -ffunction-sections -I$(SDIR)/libopencm3/include -I$(SDIR)/target/drivers/filesystems -I$(SDIR)/target/tx/radiolink/common  -fno-builtin-printf -Os --specs=nano.specs

ifeq "$(HAS_4IN1_FLASH)" "1"
CFLAGS  += -D"HAS_4IN1_FLASH=1"
endif
MODULE_CFLAGS = -fno-builtin

LFLAGS   = -nostartfiles -Wl,-gc-sections -Wl,-Map=$(TARGET).map,--cref -lc -lnosys -L$(SDIR) -L$(SDIR)/protocol -Lobjs/$(TARGET)
LFLAGS2  = -Wl,-T$(LINKFILE)

ifdef ENABLE_LTO
CFLAGS  += -flto -ffat-lto-objects -fwhole-program -DENABLE_LTO
endif

else #BUILD_TARGET

ifdef ENABLE_LTO
$(foreach src,$(filter %_isr.c,$(SRCS)),\
            $(eval CFLAGS_$(basename $(notdir $(src))) = -fno-lto))
endif

$(LIBOPENCM3):
	test -s $(SDIR)/libopencm3/Makefile || { echo "Fetch libopencm3 via 'git submodule update --init'"; exit 1; }
	+$(MAKE) -C $(SDIR)/libopencm3 TARGETS=stm32/f1 lib

$(ODIR)/devo.fs: $(LAST_MODEL) $(PRE_FS) $(TARGET).fs_wrapper
	target/drivers/filesystems/devofs/buildfs.py -c -i -f $@ -d filesystem/$(FILESYSTEM)

endif #BUILD_TARGET
