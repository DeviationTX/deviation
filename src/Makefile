# Use VERBOSE=1 to enable verbose make

TARGET   ?= devo8
PROGMODE ?= STATUS_SCREEN
include target/$(TARGET)/Makefile.inc

ifdef CROSS
    CC   = $(CROSS)gcc
    CXX  = $(CROSS)g++
    LD   = $(CROSS)ld
    AR   = $(CROSS)ar
    AS   = $(CROSS)as
    CP   = $(CROSS)objcopy
    DUMP = $(CROSS)objdump
endif
CC   ?= gcc
CXX  ?= g++
LD   ?= ld
AR   ?= ar
AS   ?= as
CP   ?= objcopy
DUMP ?= objdump

SRC_C    := $(SRC_C) $(wildcard *.c) $(wildcard screen/$(SCREENSIZE)/*.c) $(wildcard gui/*.c) $(wildcard pages/*.c) \
		$(wildcard protocol/*.c) $(wildcard config/*.c)
SRC_S    := $(SRC_S) $(wildcard *.s)
SRC_CXX  := $(SRC_CXX) $(wildcard *.cpp)
SRCS     := $(SRC_C) $(SRC_CXX) $(SRC_S)

ODIR     = objs/$(TARGET)$(ODIREXT)
OBJS 	 = $(addprefix $(ODIR)/, $(notdir $(SRC_C:.c=.o) $(SRC_S:.s=.o) $(SRC_CXX:.cpp=.o)))

CFLAGS   := $(CFLAGS) -g -Wall -Wextra -I. -D$(PROGMODE) -std=gnu99 '-DTARGET_H="target/$(TARGET)/$(TARGET)_target.h"'
CXXFLAGS := $(CXXFLAGS) $(patsubst -std=gnu99,,$(CFLAGS))
EXEEXT   ?= elf

$(TARGET).$(EXEEXT): $(LINKFILE) $(OBJS)
	@echo " + Building '$@'"
ifeq ("$(SRC_CXX)", " ")
	$(CC) -o $@ $(OBJS) $(LFLAGS) $(CFLAGS)
else
	$(CXX) -o $@ $(OBJS) $(LFLAGS) $(CFLAGS)
endif
	cp model_template.ini models/model1.ini

# The following enables quiet output unless you use VERBOSE=1
# Note that this must be after the 1st rule so that it doesn't execute by default
$(VERBOSE).SILENT:

clean:
	rm -f $(TARGET).$(EXEEXT) $(TARGET).bin $(TARGET).dfu $(TARGET).list $(TARGET).map $(ODIR)/*.o

$(OBJS): | $(ODIR)

$(ODIR):
	@mkdir -p $@

## The autodependency magic below was adapeted from:
## http://mad-scientist.net/make/autodep.html
-include $(OBJS:.o=.P)
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
	@echo " + Compiling '$$<'"
	$(CXX) $$(CXXFLAGS) -MD -c -o $$@ $$<
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

