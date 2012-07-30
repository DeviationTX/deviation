# Use VERBOSE=1 to enable verbose make

TARGET   ?= devo8
PROGMODE ?= STATUS_SCREEN
NUM_MODELS ?= 30
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

MODELDIR = filesystem/models

CFLAGS   := $(CFLAGS) -g -Wall -Wextra -I. -D$(PROGMODE) -std=gnu99 '-DTARGET_H="target/$(TARGET)/$(TARGET)_target.h"'
CXXFLAGS := $(CXXFLAGS) $(patsubst -std=gnu99,,$(CFLAGS))
EXEEXT   ?= elf

LAST_MODEL := filesystem/models/model$(NUM_MODELS).ini


.PHONY: $(LAST_MODEL)

$(TARGET).$(EXEEXT): $(LINKFILE) $(OBJS) $(LAST_MODEL)
	@echo " + Building '$@'"
ifeq ("$(SRC_CXX)", " ")
	$(CC) -o $@ $(OBJS) $(LFLAGS) $(CFLAGS)
else
	$(CXX) -o $@ $(OBJS) $(LFLAGS) $(CFLAGS)
endif

$(LAST_MODEL): model_template.ini tx_template.ini
	@echo " + Copying template files"
	cp tx_template.ini filesystem/tx.ini
	mkdir filesystem/models 2> /dev/null || true 
	echo name=Model1 > filesystem/models/model1.ini && cat model_template.ini >> filesystem/models/model1.ini
	number=2 ; while [ $$number -le 30 ] ; do \
		cp model_template.ini filesystem/models/model$$number.ini; \
		number=`expr $$number + 1`; \
		done


# The following enables quiet output unless you use VERBOSE=1
# Note that this must be after the 1st rule so that it doesn't execute by default
$(VERBOSE).SILENT:

clean:
	rm -f $(TARGET).$(EXEEXT) $(TARGET).bin $(TARGET).dfu $(TARGET).list $(TARGET).map $(ODIR)/*.o

$(OBJS): | $(ODIR)

$(ODIR):
	@mkdir -p $@

$(MODELDIR):
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

