TARGET   ?= usb_devo8
CROSS    = arm-none-eabi-

CC       = $(CROSS)gcc
CXX      = $(CROSS)g++
LD       = $(CROSS)ld
AR       = $(CROSS)ar
AS       = $(CROSS)as
CP       = $(CROSS)objcopy
DUMP     = $(CROSS)objdump

SCREEN_DIR  = ../../../screen/qvga
ROOT_DIR    = ../../../
DEVO_DIR    = ../
SRC_C    := $(SRC_C) $(wildcard *.c) $(wildcard $(DEVO_DIR)/*.c) $(wildcard $(SCREEN_DIR)/*.c) $(ROOT_DIR)/misc.c $(wildcard lib/*.c)

ODIR     = objs
OBJS 	 = $(addprefix $(ODIR)/, $(notdir $(SRC_C:.c=.o) $(SRC_S:.s=.o) $(SRC_CXX:.cpp=.o)))

CFLAGS   =  -D"assert_param(x)" -DSTM32F1 -DSTM32F10X_HD -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -std=gnu99 -fdata-sections -ffunction-sections
LFLAGS   = -nostartfiles -Wl,-gc-sections -nostartfiles -Wl,-T../devo8.ld -Wl,-Map=$(TARGET).map,--cref -lopencm3_stm32f1 -lc -lnosys

CFLAGS   := $(CFLAGS) -g -Wall -I. -I$(ROOT_DIR) -Ilib

all = $(TARGET).dfu
$(TARGET).dfu: $(TARGET).bin
	../../../../utils/dfu.py -b 0x08004000:$< $@
	../../../../utils/get_mem_usage.pl $(TARGET).map

$(TARGET).bin: $(TARGET).elf
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(TARGET).list
$(TARGET).elf: $(LINKFILE) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LFLAGS) $(CFLAGS)

clean:
	rm -f $(TARGET).elf $(TARGET).bin $(TARGET).dfu $(TARGET).list $(ODIR)/*.o

$(ODIR)/%.o: %.c
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR)/%.o: lib/%.c
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR)/%.o: $(SCREEN_DIR)/%.c
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR)/%.o: $(ROOT_DIR)/%.c
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR)/%.o: $(DEVO_DIR)/%.c
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CC) $(CFLAGS) -c -o $@ $<

# phony targets
.PHONY: clean

# recompile if the Makefile changes
$(OBJS): Makefile

