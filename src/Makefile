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

SRC_C    := $(SRC_C) $(wildcard *.c) $(wildcard screen/$(SCREENSIZE)/*.c) $(wildcard gui/*.c) $(wildcard pages/*.c)
SRC_S    := $(SRC_S) $(wildcard *.s)
SRC_CXX  := $(SRC_CXX) $(wildcard *.cpp)
SRCS     := $(SRC_C) $(SRC_CXX) $(SRC_S)

ODIR     = objs/$(TARGET)
OBJS 	 = $(addprefix $(ODIR)/, $(notdir $(SRC_C:.c=.o) $(SRC_S:.s=.o) $(SRC_CXX:.cpp=.o)))

CFLAGS   := $(CFLAGS) -g -Wall -I. -D$(PROGMODE) -std=gnu99 '-DTARGET_H="target/$(TARGET)/$(TARGET)_target.h"'

$(TARGET).elf: $(LINKFILE) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LFLAGS) $(CFLAGS)

clean:
	rm -f $(TARGET).elf $(TARGET).bin $(TARGET).dfu $(TARGET).list $(TARGET).map $(ODIR)/*.o

-include $(OBJS:.o=.P)

$(OBJS): | $(ODIR)

$(ODIR):
	@mkdir -p $@

## The autodependencymagic below was adapeted from:
## http://mad-scientist.net/make/autodep.html
dollar = $$
define define_compile_rules
$(ODIR)/%.o: $(1)%.c
	@echo " + Compiling '$$<'"
	$(CC) $$(CFLAGS) -MD -c -o $$@ $$<
	@cp $(ODIR)/$$*.d $(ODIR)/$$*.P; \
            sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$(dollar)//' \
                -e '/^$$(dollar)/ d' -e 's/$$(dollar)/ :/' < $(ODIR)/$$*.d >> $(ODIR)/$$*.P; \
            rm -f $(ODIR)/$$*.d

$(ODIR)/%.o: $(1)%.cpp
	$(CXX) $$(CFLAGS) -MD -c -o $$@ $$<
	@cp $(ODIR)/$$*.d $(ODIR)/$$*.P; \
            sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$(dollar)//' \
                -e '/^$$(dollar)/ d' -e 's/$$(dollar)/ :/' < $(ODIR)/$$*.d >> $(ODIR)/$$*.P; \
            rm -f $(ODIR)/$$*.d

endef
$(foreach directory,$(sort $(dir $(SRCS))),$(eval $(call define_compile_rules,$(directory))))

# phony targets
.PHONY: clean

# recompile if the Makefile changes
$(OBJS): Makefile

