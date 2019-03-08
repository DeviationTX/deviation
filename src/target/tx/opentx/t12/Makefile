SCREENSIZE  := 128x64x1
DFU_ARGS    := -b 0x08000000
FILESYSTEMS := common base_fonts 128x64x1
FONTS        = filesystem/$(FILESYSTEM)/media/12normal.fon \
               filesystem/$(FILESYSTEM)/media/04b03.fon
LANGUAGE    := devo10

FAMILY = opentx
TARGET = t12

CROSS    = arm-none-eabi-

LINKFILE = $(SDIR)target/tx/$(FAMILY)/$(TARGET)/$(TARGET).ld

LIBOPENCM3 = $(SDIR)libopencm3/lib/libopencm3_stm32f2.a

SRC_C = $(wildcard $(SDIR)target/tx/$(FAMILY)/$(TARGET)/*.c) \
        $(SDIR)target/drivers/display/spi/128x64x1_nt7538.c \
        $(wildcard $(SDIR)target/drivers/mcu/stm32/*.c) \
        $(wildcard $(SDIR)target/drivers/serial/soft_serial/*.c) \
        $(wildcard $(SDIR)target/drivers/serial/usb_cdc/*.c) \
        $(SDIR)target/drivers/filesystems/FatFs/ff.c \
        $(SDIR)target/drivers/filesystems/FatFs/fattime.c \
        $(SDIR)target/drivers/filesystems/FatFs/option/ccsbcs.c \
        $(wildcard $(SDIR)target/drivers/storage/mmc_flash/*.c) \
        $(wildcard $(SDIR)target/drivers/filesystems/*.c) \
        $(SDIR)target/drivers/backlight/backlight.c \
        $(SDIR)target/drivers/haptic/haptic.c \
        $(SDIR)target/drivers/rtc/rtc_driver.c \
        $(SDIR)target/drivers/input/analog/analog.c \
        $(SDIR)target/drivers/indicators/led.c \
        $(SDIR)target/drivers/input/button_switch/button_switch.c

CFLAGS   = -D"assert_param(x)=" -DSTM32F20X_HD -DSTM32F2 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fdata-sections -ffunction-sections -I$(SDIR)libopencm3/include -fno-builtin-printf -Os -I$(SDIR)target/drivers/filesystems
MODULE_FLAGS = -fno-builtin

LFLAGS   = -nostartfiles -Wl,-gc-sections -Wl,-Map=$(SDIR)$(TARGET).map,--cref -lc
LFLAGS2  = -Wl,-T$(LINKFILE)
LFLAGS2OPT  = -Wl,-Ttarget/$(TARGET)/$(TARGET)_opt.ld

.PHONY: default
default: $(SDIR)$(TARGET).bin

#-lnosys

ALL = $(LIBOPENCM3) $(SDIR)$(TARGET).bin

$(SDIR)$(TARGET).bin: $(SDIR)$(TARGET).elf
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(SDIR)$(TARGET).list

$(LIBOPENCM3): $(ODIR)
	test -s $(SDIR)libopencm3/Makefile || { echo "Fetch libopencm3 via 'git submodule update --init'"; exit 1; }
	+$(FLOCKS) $(MAKE) -C $(SDIR)libopencm3 TARGETS=stm32/f2 lib

$(SDIR)$(TARGET).dfu: $(LIBOPENCM3) $(SDIR)$(TARGET).bin
	$(SDIR)../utils/dfu.py --name "$(HGVERSION) Firmware" $(DFU_ARGS):$< $@
	$(SDIR)../utils/get_mem_usage.pl $(SDIR)$(TARGET).map

include $(SDIR)Makefile.inc
