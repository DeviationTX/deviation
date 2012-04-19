APP      = devo8

CROSS    = arm-none-eabi-
CC       = $(CROSS)gcc
LD       = $(CROSS)ld
AR       = $(CROSS)ar
AS       = $(CROSS)as
CP       = $(CROSS)objcopy
DUMP     = $(CROSS)objdump

SRC_C    = $(wildcard *.c) $(wildcard devo8/*.c)
SRC_S    = $(wildcard *.s)
OBJS 	 = $(SRC_C:.c=.o) $(SRC_S:.s=.o)

LINKFILE = devo8.ld

CFLAGS   = -DSTM32F1 -mcpu=cortex-m3 -mthumb -g
LFLAGS   = -nostartfiles --gc-sections

$(APP).dfu: $(APP).bin
	./dfu.py -b 0x08004000:$< $@

$(APP).bin: $(APP).elf
	$(CP) -O binary $< $@
	$(DUMP) -S $< > $(APP).list

#$(APP).elf: $(LINKFILE) $(OBJS)
#	$(LD) $(LFLAGS) -T $^ -o $@
#The above doesn't prune unused functions...why not?
$(APP).elf: $(LINKFILE) $(SRC_C) $(SRC_S)
	$(CC) $(CFLAGS) -o $@ -O -nostartfiles -Wl,-T$^ -lopencm3_stm32f1 -lc -lnosys

clean:
	rm -f $(APP).elf $(APP).bin $(APP).dfu $(APP).list *.o 

# phony targets
.PHONY: clean

# recompile if the Makefile changes
$(OBJS): Makefile
