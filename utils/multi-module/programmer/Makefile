## the following disable builtin rules which we don't need
## and make debugging harder
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

# Use VERBOSE=1 to enable verbose make
TARGET   ?= devo8
PROGMODE ?= STATUS_SCREEN
NUM_MODELS ?= 30
SDIR := ../../../src

include $(SDIR)/target/$(TARGET)/Makefile.inc

TYPE     ?= prd

###############################################
#This section defines binaries needed to build#
###############################################
ifdef CROSS
    CC   = $(CROSS)gcc
    CXX  = $(CROSS)g++
    LD   = $(CROSS)ld
    AR   = $(CROSS)ar
    AS   = $(CROSS)as
    CP   = $(CROSS)objcopy
    DUMP = $(CROSS)objdump
    NM   = $(CROSS)nm
endif
CC   ?= gcc
CXX  ?= g++
LD   ?= ld
AR   ?= ar
AS   ?= as
CP   ?= objcopy
DUMP ?= objdump
###############################################
#END SECTION                                  #
###############################################

############################################
#This section defines the source files used#
############################################
SRC_C    +=  $(wildcard $(SDIR)/screen/*.c) \
             $(wildcard $(SDIR)/screen/$(SCREENSIZE)/*.c) \
             $(wildcard $(SDIR)/config/*.c) \
             $(SDIR)/misc.c \
             $(SDIR)/protocol/spi/avr_program.c \
             $(wildcard *.c)


ifdef MODULAR
   SRC_C    += protocol/protocol.c
   PROTO_SRC_C := $(filter-out protocol/protocol.c, $(wildcard protocol/*.c))
else
   SRC_C    := $(SRC_C) $(wildcard protocol/*.c)
endif

SRC_C    := $(SRC_C) $(wildcard pages/$(SCREENSIZE)/standard/*.c)
SRC_C    := $(SRC_C) $(wildcard pages/$(SCREENSIZE)/advanced/*.c)

SRC_S    := $(SRC_S) $(wildcard *.s)
SRC_CXX  := $(SRC_CXX) $(wildcard *.cpp)
SRCS     := $(SRC_C) $(SRC_CXX) $(SRC_S)
############################################
#END SECTION                               #
############################################


############################################
#This section intermediate build files     #
############################################
ODIR     = objs/$(TARGET)$(ODIREXT)
OBJS 	 = $(addprefix $(ODIR)/, $(notdir $(SRC_C:.c=.o) $(SRC_S:.s=.o) $(SRC_CXX:.cpp=.o)))

ifdef MODULAR
PROTO_OBJS  := $(addprefix $(ODIR)/, $(notdir $(PROTO_SRC_C:.c=.o)))
PROTO_LIBS  := $(addprefix $(ODIR)/, $(notdir $(PROTO_SRC_C:.c=.bin)))
PROTO_EXTRA_OBJS := $(addprefix $(ODIR)/, $(notdir $(PROTO_EXTRA_C:.c=.o)))
SYMBOL_FILE := $(ODIR)/symbolfile
endif

############################################
#END SECTION                               #
############################################

##################################################
#This section contains switches used for building#
##################################################
ifeq "$(TYPE)" "dev"
  CFLAGS   := $(CFLAGS) -g -DBUILDTYPE_DEV
endif
ifdef MODULAR
CFLAGS   +=  -DMODULAR=$(MODULAR)
endif

CFLAGS   := $(CFLAGS) -Wall -Wextra -I$(SDIR) -D$(PROGMODE) -std=gnu99 -I$(SDIR)/target/$(TARGET) -I$(SDIR)/gui/$(SCREENSIZE)
CXXFLAGS := $(CXXFLAGS) $(patsubst -std=gnu99,,$(CFLAGS))
EXEEXT   ?= elf
##################################################
#END SECTION                                     #
##################################################

############################################
#this section includes build targets       #
############################################
all : $(ALL)

####################################
# recompile if the Makefile changes#
####################################
$(OBJS): Makefile ../../../src/target/$(TARGET)/Makefile.inc $(EXTRA_MAKEFILES)

##################################################################################
# The following enables quiet output unless you use VERBOSE=1                    #
# Note that this must be after the 1st rule so that it doesn't execute by default#
##################################################################################
$(VERBOSE).SILENT:

.PHONY: clean distclean language

clean:
	rm -f $(TARGET).$(EXEEXT) $(TARGET).bin $(TARGET).dfu \
		$(ODIR)/*.o $(ODIR)/*.o_ $(ODIR)/*.P

distclean:
	rm -f *.$(EXEEXT) *.bin *.dfu *.list *.map
	rm -rf objs

##########################################
#Ensure necessray directories are created#
##########################################
$(OBJS): | $(ODIR)

$(ODIR):
	@mkdir -p $@

$(MODELDIR):
	@mkdir -p $@

######################
#The main expecutable#
######################
$(TARGET).$(EXEEXT): $(LINKFILE) $(OBJS) $(LIBOPENCM3)
	@echo " + Building '$@'"
ifdef LINKFILE #Create an empty 'obj/optimize.ld' just in case the linker script needs it
	echo "" > objs/optimize.ld
endif
ifeq ("$(SRC_CXX)", " ")
	$(CC) -o $@ $(OBJS) $(LIBOPENCM3) $(LFLAGS) $(LFLAGS2) $(CFLAGS)
else
	$(CXX) -o $@ $(OBJS) $(LIBOPENCM3) $(LFLAGS) $(LFLAGS2) $(CFLAGS)
endif

##############################
#Build rules for all .o files#
##############################
## The autodependency magic below was adapeted from:
## http://mad-scientist.net/make/autodep.html
-include $(OBJS:.o=.P)
-include $(PROTO_OBJS:.o=.P)
-include $(PROTO_EXTRA_OBJS:.o=.P)

dollar = $$
define define_compile_rules
$(ODIR)/%.o: $(1)%.c
	@echo " + Compiling '$$<'"
	$(CC) $$(CFLAGS) -MD -c -o $$@ $$<
	@cp $(ODIR)/$$*.d $(ODIR)/$$*.P; \
            sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$(dollar)//' \
                -e '/^$$(dollar)/ d' -e 's/$$(dollar)/ :/' < $(ODIR)/$$*.d >> $(ODIR)/$$*.P; \
            rm -f $(ODIR)/$$*.d
endef
$(foreach directory,$(sort $(dir $(SRCS))),$(eval $(call define_compile_rules,$(directory))))

BUILD_TARGET := 1
include ../../../src/target/$(TARGET)/Makefile.inc
