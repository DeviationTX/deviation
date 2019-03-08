#
# This inc file have to be included as last include in the makefile include chain.
# Some import variables:
#     DRIVERS - list out the drivres which need be added.
#     SCREENSIZE - define the screen size (type)
#     CROSS - define if need a different compiler than default
#     CFLAGS - define the compiler options for firmware
#     LDFLAGS - define the linker options
#     SRCS_C - C source files in the full path
#     SRCS_CPP - cpp source files in the full path

# We want to turn on INCLUDE_FS by default
INCLUDE_FS ?= 1

# Use VERBOSE=1 to enable verbose make
PROGMODE ?= STATUS_SCREEN

NUM_MODELS ?= 30
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
#ifndef SCREENSIZE
#error please define SCREENSIZE
#endif

SRC_C    +=  $(wildcard $(SDIR)*.c) \
             $(wildcard $(SDIR)misc/*.c) \
             $(wildcard $(SDIR)screen/*.c) \
             $(wildcard $(SDIR)screen/$(SCREENSIZE)/*.c) \
             $(wildcard $(SDIR)gui/*.c) \
             $(wildcard $(SDIR)pages/$(SCREENSIZE)/*.c) \
             $(wildcard $(SDIR)config/*.c)

ifdef MODULAR
   SRC_C    += $(SDIR)protocol/protocol.c
   PROTO_SRC_C := $(filter-out $(SDIR)protocol/protocol.c, $(wildcard $(SDIR)protocol/*.c) $(wildcard $(SDIR)protocol/spi/*.c))
else
   SRC_C    += $(wildcard $(SDIR)protocol/*.c) $(wildcard $(SDIR)protocol/spi/*.c)
endif

SRC_C    += $(wildcard $(SDIR)pages/$(SCREENSIZE)/standard/*.c)
SRC_C    += $(wildcard $(SDIR)pages/$(SCREENSIZE)/advanced/*.c)

SRC_S    += $(wildcard *.s)
SRC_CXX  += $(wildcard *.cpp)
SRCS     := $(SRC_C) $(SRC_CXX) $(SRC_S)
############################################
#END SECTION                               #
############################################


############################################
#This section intermediate build files     #
############################################
ODIR     = $(SDIR)objs/$(TARGET)$(ODIREXT)/
OBJS    := $(patsubst %.c,$(ODIR)%.o,$(notdir $(SRCS)))

ifdef MODULAR
PROTO_OBJS  := $(addprefix $(ODIR), $(notdir $(PROTO_SRC_C:.c=.o)))
PROTO_LIBS  := $(addprefix $(ODIR), $(notdir $(PROTO_SRC_C:.c=.bin)))
PROTO_EXTRA_OBJS := $(addprefix $(ODIR), $(notdir $(PROTO_EXTRA_C:.c=.o_)))
SYMBOL_FILE := $(ODIR)symbolfile
endif

define uniq =
  $(eval seen :=)
  $(foreach _,$1,$(if $(filter $_,${seen}),,$(eval seen += $_)))
  ${seen}
endef

VPATH = $(call uniq, $(dir $(SRCS)))
$(warning $(VPATH))
HGVERSION ?= $(shell $(SDIR)../utils/get_version.pl ${TARGET})
HGVERSION := $(HGVERSION)

# Rebuild on hg version change.
# Based on http://mercurial.selenic.com/wiki/VersioningWithMake
# We need to always run this code, as opposed to running it from a prerequisite
# The HGTEST variable is not used
HGTEST := $(shell mkdir -p $(ODIR); \
	[ -f $(ODIR)hgstamp ] || touch $(ODIR)hgstamp; \
	echo $(HGVERSION) | cmp -s $(ODIR)hgstamp - \
	|| echo $(HGVERSION) > $(ODIR)hgstamp)
############################################
#END SECTION                               #
############################################

##################################################
#This section contains switches used for building#
##################################################
ifeq "$(TYPE)" "dev"
  CFLAGS   := $(CFLAGS) -DBUILDTYPE_DEV
endif
ifdef MODULAR
MODULE_CFLAGS   :=  -DMODULAR=$(MODULAR)
CFLAGS   +=  -DENABLE_MODULAR=$(MODULAR)
endif

CFLAGS   := $(CFLAGS) -g -Wall -Wextra -Werror=undef -I$(SDIR) -D$(PROGMODE) -std=gnu99 \
            -I$(SDIR)target/tx/$(FAMILY)/$(TARGET) -I$(SDIR)gui/$(SCREENSIZE) \
            -I$(SDIR)pages/$(SCREENSIZE) \
            -DHGVERSION="\"${HGVERSION}\""
CXXFLAGS := $(CXXFLAGS) $(patsubst -std=gnu99,,$(CFLAGS))
EXEEXT   ?= elf
##################################################
#END SECTION                                     #
##################################################

############################################
#this section defines final build files    #
############################################
FILESYSTEM = $(subst emu_,,$(TARGET))
MODELDIR = filesystem/$(FILESYSTEM)/models

LAST_MODEL := $(MODELDIR)/model$(NUM_MODELS).ini

ALL += $(TARGET).fs_wrapper

############################################
#END SECTION                               #
############################################

############################################
#this section includes build targets       #
############################################
.PHONY: all
all : $(warning $(ALL)) $(ALL) $(OBJS)

####################################
# recompile if the Makefile changes#
####################################
#$(OBJS) $(PROTO_OBJS) ($PROTO_EXTRA_OBJS): Makefile target/tx/$(FAMILY)/$(TARGET)/Makefile.inc $(EXTRA_MAKEFILES)

##################################################################################
# The following enables quiet output unless you use VERBOSE=1                    #
# Note that this must be after the 1st rule so that it doesn't execute by default#
##################################################################################
$(VERBOSE).SILENT:

.PHONY: clean language lint

clean:
	rm -f $(TARGET).$(EXEEXT) $(TARGET).exe $(TARGET).bin $(TARGET).dfu $(TARGET).list \
		$(TARGET).map $(ODIR)*.o $(ODIR)*.o_ $(ODIR)*.P  $(ODIR)*.bin \
		filesystem/$(FILESYSYTEM) 2> /dev/null || true

language:
	$(SDIR)../utils/extract_strings.pl -po > $(SDIR)fs/language/deviation.po


zip: $(TARGET).zip

%.zip: $(ALL) $(TARGET).dfu $(PROTO_MODULES)
	#This is not an emulator build (emulator is hanled in target/common/emu/Makefile.inc)
	cp -f $(TARGET).dfu deviation-$(HGVERSION).dfu
	rm -f deviation-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	zip debug-$(HGVERSION).zip $(TARGET).elf
	# $(MAKE) zipping more compatible with other distributions
	export p=`pwd`; cd $(ODIR) && test -e devo.fs && zip -u $$p/deviation-$(HGVERSION).zip devo.fs && rm devo.fs || true
	zip deviation-$(HGVERSION).zip deviation-$(HGVERSION).dfu UPDATING.md
ifdef MODULAR
	export p=`pwd`; cd $(ODIR) && zip -ur $$p/deviation-$(HGVERSION).zip protocol/*.mod
	export p=`pwd`; cd $(ODIR) && zip -u $$p/debug-$(HGVERSION).zip *.elf
	rm -rf $(ODIR)protocol
endif
ifeq "$(INCLUDE_FS)" "1"
	export p=`pwd`; cd filesystem/$(FILESYSTEM) && zip -ur $$p/deviation-$(HGVERSION).zip  *
endif
	zip -u deviation-$(HGVERSION).zip debug-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	rm -f deviation-$(HGVERSION).dfu

fs: $(TARGET).fs

%.fs:
	rm -f deviation-fs-$(HGVERSION).zip; zip -r deviation-fs-$(HGVERSION).zip filesystem/$(FILESYSTEM)

coverity:
	rm -rf cov-int 2>/dev/null; $(MAKE) distclean
	cov-build --dir cov-int make TARGET=devo8
	cov-build --dir cov-int make TARGET=devo10
	tar -cvzf coverity-$(HGVERSION).tgz cov-int

lint:
	$(SDIR)../utils/run_linter.py --diff --debug

##########################################
#Ensure necessary directories are created#
##########################################
$(OBJS): | $(ODIR)

$(ODIR):
	@mkdir -p $@

$(MODELDIR):
	@mkdir -p $@

######################
#The main executable #
######################
$(SDIR)$(TARGET).$(EXEEXT): $(LINKFILE) $(OBJS) $(LIBOPENCM3)
	@echo " + Building '$@'"
ifdef LINKFILE #Create an empty 'obj/$(TARGET)/optimize.ld' just in case the linker script needs it
	echo "" > $(ODIR)optimize.ld
endif
ifeq ("$(SRC_CXX)", " ")
	$(CC) -o $@ $(OBJS) $(LIBOPENCM3) $(LFLAGS) $(CFLAGS) $(EXTRA_CFLAGS)
else
	$(CXX) -o $@ $(OBJS) $(LIBOPENCM3) $(LFLAGS) $(CFLAGS) $(EXTRA_CFLAGS)
endif


##############################
#Build rules for all .o files#
##############################
$(ODIR)%.o: %.c
	@echo " + Compiling '$<'"
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$(basename $(notdir $<))) -c -o $@ $<

$(ODIR)%.o: %.cpp
	@echo " + Compiling '$<'"
	$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(CXXFLAGS_$(basename $(notdir $<))) -c -o $@ $<

#For module building
$(ODIR)%.o_: %.c
	@echo " + Compiling '$<' as module"
	$(CC) $(CFLAGS) $(MODULE_CFLAGS) $(CFLAGS_$(basename $(notdir $<))) -c -o $@ $<

##############################
#Ensure version is up to date#
##############################
# Rebuild on hg version change.
$(ODIR)version.o: $(ODIR)hgstamp

$(ODIR)hgstamp:
	echo $(HGVERSION) > $(ODIR)hgstamp

include $(SDIR)fs/Makefile.inc