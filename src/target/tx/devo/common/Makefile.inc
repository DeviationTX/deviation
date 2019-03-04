HAS_4IN1_FLASH ?= 0
HAS_FLASH_DETECT ?= 0
USE_JTAG ?= 0

ifndef BUILD_TARGET

CROSS    = arm-none-eabi-
EXTRA_MAKEFILES := $(EXTRA_MAKEFILES) $(SDIR)/target/tx/devo/common/Makefile.inc

PROTO_LINKFILE = $(SDIR)/target/tx/devo/$(TARGET)/protocol.ld

LIBOPENCM3 = $(SDIR)/libopencm3/lib/libopencm3_stm32f1.a

SRC_C    = $(wildcard $(SDIR)/target/tx/devo/$(TARGET)/*.c) \
           $(wildcard $(SDIR)/target/drivers/mcu/stm32/*.c) \
           $(wildcard $(SDIR)/target/drivers/storage/*.c) \
           $(wildcard $(SDIR)/target/drivers/indicators/*.c) \
           $(wildcard $(SDIR)/target/tx/devo/common/*.c) \
           $(addprefix $(SDIR)/target/drivers/display/, $(DISPLAY_DRIVER)) \
           $(wildcard $(SDIR)/target/drivers/filesystems/*.c) \
           $(wildcard $(SDIR)/target/drivers/filesystems/petit_fat/*.c) \
           $(wildcard $(SDIR)/target/drivers/filesystems/devofs/*.c) \
           $(wildcard $(SDIR)/target/drivers/input/button_matrix/*.c) \
           $(wildcard $(SDIR)/target/drivers/serial/uart/*.c) \
           $(wildcard $(SDIR)/target/drivers/serial/soft_serial/*.c) \
           $(wildcard $(SDIR)/target/drivers/sound/piezo/*.c) \
           $(wildcard $(SDIR)/target/drivers/sound/uart_audio/*.c) \
           $(wildcard $(SDIR)/target/drivers/usb/*.c) \
           $(SDIR)/target/drivers/backlight/backlight.c \
           $(SDIR)/target/drivers/rtc/rtc_driver.c \
           $(SDIR)/target/drivers/haptic/haptic.c \
           $(SDIR)/target/drivers/input/analog/analog.c

ifdef MODULAR
PROTO_EXTRA_C = $(wildcard $(SDIR)/target/tx/devo/common/protocol/*.c) \
                $(SDIR)/target/drivers/usb/devo_hid.c \
                $(wildcard $(SDIR)/protocol/spi/*.c) \
                $(SDIR)/target/tx/devo/common/module_signature.c \
                $(SDIR)/target/tx/devo/common/uart.c
else
SRC_C   += $(wildcard $(SDIR)/target/tx/devo/common/protocol/*.c)
endif

CFLAGS   = -D"assert_param(x)=" -DSTM32F10X_HD -DSTM32F1 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fdata-sections -I$(SDIR)/target/tx/devo/common -I$(SDIR)/libopencm3/include -I$(SDIR)/target/drivers/filesystems -fno-builtin-printf -Os --specs=nano.specs
ifeq "$(HAS_4IN1_FLASH)" "1"
CFLAGS  += -D"HAS_4IN1_FLASH=1"
endif
ifeq "$(HAS_FLASH_DETECT)" "1"
CFLAGS  += -D"HAS_FLASH_DETECT=1"
endif
ifeq "$(USE_JTAG)" "1"
CFLAGS  += -D"USE_JTAG=1"
endif
MODULE_CFLAGS := -fno-builtin

#LFLAGS   = -nostartfiles -Wl,-gc-sections -Wl,-Map=$(TARGET).map,--cref -nostdlib
LFLAGS   = -nostartfiles -Wl,-gc-sections -Wl,-Map=$(TARGET).map,--cref -lc -lnosys -L$(SDIR) -L$(SDIR)/protocol -Lobjs/$(TARGET) -Wl,-warn-common

LINKFILE = $(SDIR)/target/tx/devo/$(TARGET)/$(TARGET).ld
LFLAGS2  = -Wl,-T$(LINKFILE)

ifdef ENABLE_LTO
EXTRA_CFLAGS  := $(EXTRA_CFLAGS) -flto -ffat-lto-objects -DENABLE_LTO
MODULE_CFLAGS := $(MODULE_CFLAGS) -ffunction-sections
else
CFLAGS += -ffunction-sections
endif

ifdef OPTIMIZE_DFU
LINKFILEOPT = $(SDIR)/target/tx/devo/$(TARGET)/$(TARGET)_opt.ld
ifeq (,$(wildcard $(LINKFILEOPT)))
LINKFILEOPT = $(LINKFILE)
endif
LFLAGS2OPT  = -Wl,-T$(LINKFILEOPT)
endif

# Enable warning as error
CFLAGS += -Werror

# Enable stack usage analyze
ifeq "$(TYPE)" "stack"
CFLAGS += -fstack-usage -Wno-error -Wstack-usage=1024
endif

#-lnosys

ALL = $(LIBOPENCM3) $(TARGET).dfu

else #BUILD_TARGET

ifdef ENABLE_LTO
$(foreach src,$(filter %_isr.c,$(SRCS)),\
            $(eval CFLAGS_$(basename $(notdir $(src))) = -fno-lto))
endif

ifeq ($(OS),Windows_NT)
	FLOCKS =
else
	FLOCKS = flock objs/.lock
endif

$(TARGET).bin: $(TARGET).elf
ifdef OPTIMIZE_DFU
	$(CP) $(TARGET).elf objs/$(TARGET)/$(TARGET).elf
	@echo " + Optimizing placement and re-linking"
	$(SDIR)/../utils/repack_ld.pl -mapfile $(TARGET).map -linkfile $(LINKFILE) > objs/$(TARGET)/optimize.ld
	$(CC) -o $< $(OBJS) $(LIBOPENCM3) $(LFLAGS) $(LFLAGS2OPT) $(CFLAGS)
endif
ifdef MODULAR # the following builds a symbol file with thumb addresses (address+1 for functions, address+0 for variables)
	$(DUMP) -t $< | $(SDIR)/../utils/build_symbolfile.pl > $(SYMBOL_FILE)
endif
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(TARGET).list

$(LIBOPENCM3):
	test -s $(SDIR)/libopencm3/Makefile || { echo "Fetch libopencm3 via 'git submodule update --init'"; exit 1; }
	+$(FLOCKS) $(MAKE) -C $(SDIR)/libopencm3 TARGETS=stm32/f1 lib

$(TARGET).dfu: $(TARGET).bin
	$(SDIR)/../utils/dfu.py --name "$(HGVERSION) Firmware" $(DFU_ARGS):$< $@
	$(SDIR)/../utils/get_mem_usage.pl $(TARGET).map

###################################
#The protocol modules (if needed) #
###################################
$(ODIR)/%.bin: $(ODIR)/%.o_ $(SDIR)/protocol/exports.ld $(PROTO_LINKFILE) $(PROTO_EXTRA_OBJS) $(TARGET).bin
	@echo " + Building '$@'"
	$(LD) -o $(@:.bin=.elf) $< $(PROTO_EXTRA_OBJS) $(LIBOPENCM3) -gc-sections -Map=$(@:.bin=.map) \
		--cref -T$(PROTO_LINKFILE) --just-symbols=$(SYMBOL_FILE)
	$(DUMP) -t $(@:.bin=.elf) | grep -q _from_thumb; if [ $$? -eq 0 ]; then echo "ERROR: Bad address found"; false; else true; fi
	$(CP) -O binary  $(@:.bin=.elf) $@

endif #BUILD_TARGET
