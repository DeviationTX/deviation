TARGET   ?= devo8
PROGMODE ?= STATUS_SCREEN
include target/$(TARGET)/Makefile.inc

CC       = $(CROSS)gcc
CXX      = $(CROSS)g++
LD       = $(CROSS)ld
AR       = $(CROSS)ar
AS       = $(CROSS)as
CP       = $(CROSS)objcopy
DUMP     = $(CROSS)objdump

SRC_C    := $(SRC_C) $(wildcard *.c) $(wildcard screen/$(SCREENSIZE)/*.c)
SRC_S    := $(SRC_S) $(wildcard *.s)
SRC_CXX  := $(SRC_CXX) $(wildcard *.cpp)

ODIR     = objs/$(TARGET)
OBJS 	 = $(addprefix $(ODIR)/, $(notdir $(SRC_C:.c=.o) $(SRC_S:.s=.o) $(SRC_CXX:.cpp=.o)))

CFLAGS   := $(CFLAGS) -g -Wall -I. -D$(PROGMODE) -std=gnu99

$(TARGET).elf: $(LINKFILE) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LFLAGS) $(CFLAGS)

clean:
	rm -f $(TARGET).elf $(TARGET).bin $(TARGET).dfu $(TARGET).list $(ODIR)/*.o

$(ODIR)/%.o: %.c
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR)/%.o: target/$(TARGET)/%.c
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR)/%.o: target/$(TARGET)/%.cpp
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CXX) $(CFLAGS) -c -o $@ $<

$(ODIR)/%.o: common/%.c
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR)/%.o: screen/$(SCREENSIZE)/%.c
	mkdir -p $(ODIR) 2>/dev/null || true
	$(CC) $(CFLAGS) -c -o $@ $<


# phony targets
.PHONY: clean

# recompile if the Makefile changes
$(OBJS): Makefile

