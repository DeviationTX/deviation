FILESYSTEM := x9d
SCREENSIZE := 128x64x1
DFU_ARGS   := -b 0x08000000

ifndef BUILD_TARGET

CROSS    = arm-none-eabi-

LINKFILE = target/$(TARGET)/$(TARGET).ld

LIBOPENCM3 = libopencm3/lib/libopencm3_stm32f2.a

SRC_C    = $(wildcard target/$(TARGET)/*.c) \
	target/common_devo/printf.c \
	target/common_devo/fgets.c \
	target/common_devo/syscalls.c \
	target/common_devo/petit_io.c \
	target/common_devo/petit_fat.c \
	target/common_devo/iwdg.c \
	target/common_devo/err_handler.c

SRC_C   += $(wildcard target/common_devo/protocol/*.c)

CFLAGS   = -D"assert_param(x)=" -DSTM32F20X_HD -DSTM32F2 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -fdata-sections -ffunction-sections -Ilibopencm3/include -fno-builtin-printf -Os
MODULE_FLAGS = -fno-builtin

LFLAGS   = -nostartfiles -Wl,-gc-sections -Wl,-Map=$(TARGET).map,--cref -lc
LFLAGS2  = -Wl,-T$(LINKFILE)
LFLAGS2OPT  = -Wl,-Ttarget/$(TARGET)/$(TARGET)_opt.ld

#-lnosys

ALL = $(TARGET).dfu

else #BUILD_TARGET

$(TARGET).bin: $(TARGET).elf
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(TARGET).list

$(LIBOPENCM3):
	make -C libopencm3 TARGETS=stm32/f2 lib

$(TARGET).dfu: $(TARGET).bin
	../utils/dfu.py --name "$(HGVERSION) Firmware" $(DFU_ARGS):$< $@
	../utils/get_mem_usage.pl $(TARGET).map

endif #BUILD_TARGET
