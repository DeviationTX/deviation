## and make debugging harder
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

###########################################
## Provide some convenient make targets. ##
###########################################
ifndef  TARGET
TARGET   ?= devo8
default: $(TARGET)

ALLTARGETS = $(filter-out common,$(notdir $(wildcard target/*)))

# The supported transmitters
TXS ?= devo6 devo7e devo8 devo10 devo12 devof7 devof4 devof12e
# These txs use file systems for other txs, so don't have a file system target.
NOFSTXS = devo6 devo7e

#Filter non-existant emus
EMUS = $(foreach dir,$(TXS:%=emu_%),$(if $(wildcard target/$(dir)),$(dir),))
ALLEMUS = $(foreach dir,$(ALLTXS:%=emu_%),$(if $(wildcard target/$(dir)),$(dir),))

txs: $(TXS)
emus: $(EMUS)
both: txs emus
fss: $(TXS:%=fs_%)
zips: $(TXS:%=zip_%)
emuzips: $(EMUS:%=zip_%)
winzips: $(EMUS:%=zip_win_%)
everything: txs emus fss

# Generate the targets for a transmitter.
define make-target
$1:
	$(MAKE) TARGET=$1
# This builds Windows emulators on a Linux system.
win_$1:
	$(MAKE) TARGET=$1 WINDOWS=1 CROSS=i586-mingw32msvc-
fs_$1:
ifneq "$(INCLUDE_FS)" "1"
ifneq "$(findstring $1,$(NOFSTXS))" "$1"
	$(MAKE) TARGET=$1 fs
endif
endif
both_$1: $1 emu_$1
zip_$1:
	$(MAKE) TARGET=$1 zip
zip_win_$1:
	$(MAKE) TARGET=$1 WINDOWS=1 CROSS=i586-mingw32msvc- zip
release_$1: zip_$1 zip_win_$1 fs_$1
all_$1:
	$(MAKE) TARGET=$1 all
endef

# Now generate all the convenience targets
$(foreach t,$(ALLTARGETS),$(eval $(call make-target,$t)))
endif

# We want to turn on INCLUDE_FS by default
INCLUDE_FS ?= 1

# Use VERBOSE=1 to enable verbose make
PROGMODE ?= STATUS_SCREEN
SDIR := .
include target/$(TARGET)/Makefile.inc

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
SRC_C    +=  $(wildcard *.c) $(wildcard screen/*.c) $(wildcard screen/$(SCREENSIZE)/*.c) \
             $(wildcard gui/*.c) $(wildcard pages/$(SCREENSIZE)/*.c) $(wildcard config/*.c)

ifdef MODULAR
   SRC_C    += protocol/protocol.c
   PROTO_SRC_C := $(filter-out protocol/protocol.c, $(wildcard protocol/*.c) $(wildcard protocol/spi/*.c))
else
   SRC_C    := $(SRC_C) $(wildcard protocol/*.c) $(wildcard protocol/spi/*.c)
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

HGVERSION := $(shell ../utils/get_version.pl ${TARGET})

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

CFLAGS   := $(CFLAGS) -Wall -Wextra -Werror=undef -I. -D$(PROGMODE) -std=gnu99 -Itarget/$(TARGET) -Igui/$(SCREENSIZE) \
            -Ipages/$(SCREENSIZE) \
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
		$(TARGET).map $(ODIR)/*.o $(ODIR)/*.o_ $(ODIR)/*.P  $(ODIR)/*.bin \
		filesystem/$(FILESYSYTEM) 2> /dev/null || true

distclean:
	rm -f *.$(EXEEXT) *.bin *.dfu *.list *.map
	rm -rf objs
	rm -rf  filesystem
	$(MAKE) -C libopencm3 clean > /dev/null
	rm -rf deviation*.zip

language:
	../utils/extract_strings.pl -update


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
	rm -rf $(ODIR)/protocol
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

release:  $(TXS:%=release_%)

coverity:
	rm -rf cov-int 2>/dev/null; $(MAKE) distclean
	cov-build --dir cov-int make TARGET=devo8
	cov-build --dir cov-int make TARGET=devo10
	tar -cvzf coverity-$(HGVERSION).tgz cov-int
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

#################
#The filesystem #
#################
%.fs_wrapper: $(LAST_MODEL)
	true

.PHONY: $(PRE_FS) $(LAST_MODEL)
$(LAST_MODEL): model_template.ini tx_template.ini $(FONTS) $(PRE_FS)
	@echo " + Copying template files for $(FILESYSTEM)"
	mkdir -p filesystem/$(FILESYSTEM) || true
	for i in $(FILESYSTEMS); do cp -prf fs/$$i/* filesystem/$(FILESYSTEM)/; done
	rm filesystem/$(FILESYSTEM)/*/.ignore 2>/dev/null || true
	cp tx_template.ini filesystem/$(FILESYSTEM)/tx.ini
	echo 'empty' > filesystem/$(FILESYSTEM)/errors.txt
	mkdir filesystem/$(FILESYSTEM)/models 2> /dev/null || true
	echo 'name=Model1' > filesystem/$(FILESYSTEM)/models/model1.ini \
		&& cat model_template.ini >> filesystem/$(FILESYSTEM)/models/model1.ini
	cp model_template.ini filesystem/$(FILESYSTEM)/models/default.ini
ifdef LANGUAGE
	mkdir filesystem/$(FILESYSTEM)/language 2> /dev/null; \
               ../utils/extract_strings.pl -fs $(FILESYSTEM) -targets $(LANGUAGE) -update -objdir $(ODIR)
endif
	export tx=$(FILESYSTEM); \
	number=2 ; while [ $$number -le $(NUM_MODELS) ] ; do \
		cp model_template.ini filesystem/$$tx/models/model$$number.ini; \
		number=`expr $$number + 1`; \
		done
	@echo " + Checking string list length for $(FILESYSTEM)"
	../utils/check_string_size.pl -target $(FILESYSTEM) -objdir $(ODIR) -quiet

######################
#Necessary Font files#
######################
filesystem/$(FILESYSTEM)/media/12normal.fon: fonts/12normal.bdf
	mkdir -p filesystem/$(FILESYSTEM)/media/
	../utils/font/bdf_to_font.pl -maxsize 12 -mode bin $< -out $@ -minspace 10

filesystem/$(FILESYSTEM)/media/15normal.fon: fonts/15normal.bdf
	mkdir -p filesystem/$(FILESYSTEM)/media/
	../utils/font/bdf_to_font.pl -maxsize 15 -mode bin $< -out $@ -minspace 8

filesystem/$(FILESYSTEM)/media/23bold.fon: fonts/23bold.bdf
	mkdir -p filesystem/$(FILESYSTEM)/media/
	../utils/font/bdf_to_font.pl -maxsize 23 -mode bin $< -out $@ -minspace 8

filesystem/$(FILESYSTEM)/media/04b03.fon: fonts/04b03.bdf
	mkdir -p filesystem/$(FILESYSTEM)/media/
	../utils/font/bdf_to_font.pl -maxsize 5 -mode bin $< -out $@ -minspace 5

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

#For module building
$(ODIR)/%.o_: $(1)%.c
	@echo " + Compiling '$$<' as module"
	$(CC) $$(CFLAGS) $$(MODULE_FLAGS) -MD -c -o $$@ $$<
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
