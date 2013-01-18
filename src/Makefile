## the following disable builtin rules which we don't need
## and make debugging harder
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

# Use VERBOSE=1 to enable verbose make
TYPE     ?= prd
TARGET   ?= devo8
PROGMODE ?= STATUS_SCREEN
NUM_MODELS ?= 30

include target/$(TARGET)/Makefile.inc

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
SRC_C    +=  $(wildcard *.c) $(wildcard screen/*.c) $(wildcard screen/$(SCREENSIZE)/*.c) \
             $(wildcard gui/*.c) $(wildcard pages/$(SCREENSIZE)/*.c) $(wildcard config/*.c)

ifdef MODULAR
   SRC_C    += protocol/protocol.c
   PROTO_SRC_C := $(filter-out protocol/protocol.c, $(wildcard protocol/*.c))
else
   SRC_C    := $(SRC_C) $(wildcard protocol/*.c)
endif

ifndef NO_STANDARD_GUI
   SRC_C    := $(SRC_C) $(wildcard pages/$(SCREENSIZE)/simple/*.c)
endif
ifndef NO_ADVANCED_GUI
   SRC_C    := $(SRC_C) $(wildcard pages/$(SCREENSIZE)/advanced/*.c)
endif

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

ifeq "$(wildcard ../.hg )" ""
  HGVERSION := ${TARGET}-Unknown
else
  ifeq "$(shell hg parents --template {latesttagdistance})" "1"
    HGVERSION := $(shell hg parents --template '${TARGET}-{latesttag}')
  else
    HGVERSION := $(shell hg parents --template '${TARGET}-{node|short}')
  endif
endif

# Rebuild on hg version change.
# Based on http://mercurial.selenic.com/wiki/VersioningWithMake
# We need to always run this code, as opposed to running it from a prerequisite
# The HGTEST variable is not used
HGTEST := $(shell mkdir -p $(ODIR); \
	[ -f $(ODIR)/hgstamp ] || touch $(ODIR)/hgstamp; \
	echo $(HGVERSION) | cmp -s $(ODIR)/hgstamp - \
	|| echo $(HGVERSION) > $(ODIR)/hgstamp)
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

CFLAGS   := $(CFLAGS) -Wall -Wextra -I. -D$(PROGMODE) -std=gnu99 -Itarget/$(TARGET) -Igui/$(SCREENSIZE) -DHGVERSION="\"${HGVERSION}\""
CXXFLAGS := $(CXXFLAGS) $(patsubst -std=gnu99,,$(CFLAGS))
EXEEXT   ?= elf
##################################################
#END SECTION                                     #
##################################################

############################################
#this section defines final build files    #
############################################

FONTS    = filesystem/devo10/media/12normal.fon \
           filesystem/devo8/media/15normal.fon \
           filesystem/devo8/media/23bold.fon
MODELDIR = filesystem/$(FILESYSTEM)/models

LAST_MODEL := $(MODELDIR)/model$(NUM_MODELS).ini

ALL += $(LAST_MODEL)
ifdef MODULAR
include protocol/Makefile.inc
endif
############################################
#END SECTION                               #
############################################

############################################
#this section includes build targets       #
############################################
all : $(ALL)

####################################
# recompile if the Makefile changes#
####################################
$(OBJS) $(PROTO_OBJS) ($PROTO_EXTRA_OBJS): Makefile target/$(TARGET)/Makefile.inc $(EXTRA_MAKEFILES)

##################################################################################
# The following enables quiet output unless you use VERBOSE=1                    #
# Note that this must be after the 1st rule so that it doesn't execute by default#
##################################################################################
$(VERBOSE).SILENT:

.PHONY: clean distclean language

clean:
	rm -f $(TARGET).$(EXEEXT) $(TARGET).bin $(TARGET).dfu $(TARGET).list \
		$(TARGET).map $(ODIR)/*.o $(ODIR)/*.P  $(ODIR)/*.bin

distclean:
	rm -f *.$(EXEEXT) *.bin *.dfu *.list *.map
	rm -rf objs
	make -C libopencm3 clean > /dev/null
	rm -rf deviation*.zip

language:
	../utils/extract_strings.pl -update


zip: $(TARGET).dfu $(PROTO_MODULES)
	mv -f $(TARGET).dfu deviation-$(HGVERSION).dfu
	rm -f deviation-$(HGVERSION).zip
	# make zipping more compatible with other distributions
	zip deviation-$(HGVERSION).zip deviation-$(HGVERSION).dfu
ifdef MODULAR
	export p=`pwd`; cd $(ODIR) && zip -ur $$p/deviation-$(HGVERSION).zip protocol/*.mod
	rm -rf $(ODIR)/protocol
endif
	rm -f deviation-$(HGVERSION).dfu

fs:
	rm -f deviation-fs-$(HGVERSION).zip; zip -r deviation-fs-$(HGVERSION).zip filesystem/$(TARGET)

release:
	make TARGET=devo8 zip
	make TARGET=devo6 zip
	make TARGET=devo10 zip
	make TARGET=devo7e zip
	make TARGET=devo8 fs
	make TARGET=devo10 fs
	#make TARGET=emu_devo8 TYPE=prd WINDOWS=1 

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
	/bin/rm -f objs/optimize.ld
	echo "" > objs/optimize.ld
endif
ifeq ("$(SRC_CXX)", " ")
	$(CC) -o $@ $(OBJS) $(LIBOPENCM3) $(LFLAGS) $(LFLAGS2) $(CFLAGS)
else
	$(CXX) -o $@ $(OBJS) $(LIBOPENCM3) $(LFLAGS) $(LFLAGS2) $(CFLAGS)
endif

#################
#The filesystem #
#################
$(LAST_MODEL): model_template.ini tx_template.ini $(FONTS)
	@echo " + Copying template files for $(FILESYSTEM)"
	export tx=$(FILESYSTEM); \
	cp -prf filesystem/common/media filesystem/$$tx/; \
	cp -prf filesystem/common/template filesystem/$$tx/; \
	mkdir filesystem/$$tx/language 2> /dev/null || true; \
               ../utils/extract_strings.pl -target $$tx; \
	cp tx_template.ini filesystem/$$tx/tx.ini; \
	mkdir filesystem/$$tx/models 2> /dev/null || true; \
	echo 'name=Model1' > filesystem/$$tx/models/model1.ini; \
	cat model_template.ini >> filesystem/$$tx/models/model1.ini; \
	cp model_template.ini filesystem/$$tx/models/default.ini; \
	number=2 ; while [ $$number -le 30 ] ; do \
		cp model_template.ini filesystem/$$tx/models/model$$number.ini; \
		number=`expr $$number + 1`; \
		done

######################
#Necessary Font files#
######################
filesystem/devo10/media/12normal.fon: fonts/12normal.bdf
	../utils/bdf_to_font.pl -maxsize 12 -mode bin $< -out $@ -minspace 10

filesystem/devo8/media/15normal.fon: fonts/15normal.bdf
	../utils/bdf_to_font.pl -maxsize 15 -mode bin $< -out $@ -minspace 8

filesystem/devo8/media/23bold.fon: fonts/23bold.bdf
	../utils/bdf_to_font.pl -maxsize 23 -mode bin $< -out $@ -minspace 8

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

$(ODIR)/%.o: $(1)%.cpp
	@echo " + Compiling '$$<'"
	$(CXX) $$(CXXFLAGS) -MD -c -o $$@ $$<
	@cp $(ODIR)/$$*.d $(ODIR)/$$*.P; \
            sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$(dollar)//' \
                -e '/^$$(dollar)/ d' -e 's/$$(dollar)/ :/' < $(ODIR)/$$*.d >> $(ODIR)/$$*.P; \
            rm -f $(ODIR)/$$*.d

endef
$(foreach directory,$(sort $(dir $(SRCS) $(PROTO_EXTRA_C))),$(eval $(call define_compile_rules,$(directory))))

##############################
#Ensure version is up to date#
##############################
# Rebuild on hg version change.
$(ODIR)/version.o: $(ODIR)/hgstamp

$(ODIR)/hgstamp:
	echo $(HGVERSION) > $(ODIR)/hgstamp

BUILD_TARGET := 1
include target/$(TARGET)/Makefile.inc
ifdef MODULAR
include protocol/Makefile.inc
endif
