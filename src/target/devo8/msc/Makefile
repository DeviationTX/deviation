TARGET   ?= usb_devo8
CROSS    = arm-none-eabi-

CC       = $(CROSS)gcc
CXX      = $(CROSS)g++
LD       = $(CROSS)ld
AR       = $(CROSS)ar
AS       = $(CROSS)as
CP       = $(CROSS)objcopy
DUMP     = $(CROSS)objdump

SRC_C    := $(SRC_C) $(wildcard *.c)
SRC_S    := $(SRC_S) $(wildcard *.s)
SRC_CXX  := $(SRC_CXX) $(wildcard *.cpp)

ODIR     = objs
OBJS 	 = $(addprefix $(ODIR)/, $(notdir $(SRC_C:.c=.o) $(SRC_S:.s=.o) $(SRC_CXX:.cpp=.o)))

CFLAGS   = -DSTM32F1 -mcpu=cortex-m3 -mthumb -mfix-cortex-m3-ldrd -std=gnu99
LFLAGS   = -nostartfiles --gc-sections -nostartfiles -Wl,-Tdevo8.ld -lopencm3_stm32f1 -lc -lnosys

CFLAGS   := $(CFLAGS) -g -Wall -I. -Ilib/inc

all = $(TARGET).dfu
$(TARGET).dfu: $(TARGET).bin
	../../../../utils/dfu.py -b 0x08004000:$< $@

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

$(ODIR)/%.o: lib/src/%.c
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CC) $(CFLAGS) -c -o $@ $<


# phony targets
.PHONY: clean

# recompile if the Makefile changes
$(OBJS): Makefile

