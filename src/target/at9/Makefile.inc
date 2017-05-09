HAS_4IN1_FLASH ?= 0
HAS_FLASH_DETECT ?= $(HAS_4IN1_FLASH)

ifeq "$(HAS_4IN1_FLASH)" "1"
FILESYSTEMS := common base_fonts 320x240x16
SCREENSIZE  := 320x240x16
FONTS        = filesystem/$(FILESYSTEM)/media/15normal.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon
else
FILESYSTEMS := common base_fonts 320x240x1
SCREENSIZE  := 320x240x16
FONTS        = filesystem/$(FILESYSTEM)/media/23bold.fon
endif

DFU_ARGS    := -D 0x0483:0xdf12 -b 0x08003000

ifndef BUILD_TARGET
CROSS    = arm-none-eabi-

ifneq "$(HAS_4IN1_FLASH)" "1"
EXTRA_DFUS = $(TARGET)-fs.dfu
endif

ALL = $(LIBOPENCM3) $(TARGET).dfu $(EXTRA_DFUS)

ifeq "$(HAS_4IN1_FLASH)" "1"
NUM_MODELS ?= 30
else
NUM_MODELS ?= 10
endif

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
           $(wildcard $(SDIR)/target/common/devo/uart.c) \
           $(SDIR)/target/devo8/ili9341.c

ifneq "$(HAS_4IN1_FLASH)" "1"
SRC_C := $(filter-out $(SDIR)/target/common/stm32/spi_flash.c, $(SRC_C))
endif

CFLAGS   = -D"assert_param(x)=" -DSTM32F10X_HD -DSTM32F1 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fdata-sections -ffunction-sections -I$(SDIR)/target/common/devo/msc2/lib -I$(SDIR)/target/common/devo/msc2 -I$(SDIR)/libopencm3/include -I$(SDIR)/target/common/filesystems -fno-builtin-printf -Os --specs=nano.specs
ifeq "$(HAS_4IN1_FLASH)" "1"
CFLAGS  += -D"HAS_4IN1_FLASH=1"
endif
ifeq "$(HAS_FLASH_DETECT)" "1"
CFLAGS  += -D"HAS_FLASH_DETECT=1"
endif
MODULE_FLAGS = -fno-builtin

LFLAGS   = -nostartfiles -Wl,-gc-sections -Wl,-Map=$(TARGET).map,--cref -lc -lnosys -L$(SDIR)
LFLAGS2  = -Wl,-T$(LINKFILE)


else #BUILD_TARGET

$(TARGET).bin: $(TARGET).elf
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(TARGET).list

$(LIBOPENCM3):
	$(MAKE) -C $(SDIR)/libopencm3 TARGETS=stm32/f1 lib

$(TARGET).dfu: $(TARGET).bin
	$(SDIR)/../utils/dfu.py --name "$(HGVERSION) Firmware" $(DFU_ARGS):$< $@
	$(SDIR)/../utils/get_mem_usage.pl $(TARGET).map

$(ODIR)/devo.fs: $(LAST_MODEL) $(TARGET).dfu $(PRE_FS) $(TARGET).fs_wrapper
	target/common/filesystems/devofs/buildfs.py -c -i -f $@ -d filesystem/$(FILESYSTEM)

$(TARGET)-fs.dfu: $(ODIR)/devo.fs
	$(SDIR)/../utils/dfu.py --name "$(HGVERSION) Filesystem" -D 0x0483:0xdf12 -b 0x08040000:$< $@

$(TARGET).fs_wrapper: $(LAST_MODEL)
	rm filesystem/$(FILESYSTEM)/datalog.bin

$(TARGET).zip: $(ALL)
	cp -f $(TARGET).dfu deviation-$(HGVERSION).dfu
	rm -f deviation-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	zip debug-$(HGVERSION).zip $(TARGET).elf
	# make zipping more compatible with other distributions
	zip deviation-$(HGVERSION).zip deviation-$(HGVERSION).dfu $(EXTRA_DFUS)
	zip -u deviation-$(HGVERSION).zip debug-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	rm -f deviation-$(HGVERSION).dfu

endif #BUILD_TARGET
