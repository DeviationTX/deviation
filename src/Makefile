## the following disable builtin rules which we don't need
## and make debugging harder
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

# Use VERBOSE=1 to enable verbose make
TYPE     ?= dev
TARGET   ?= devo8
PROGMODE ?= STATUS_SCREEN
NUM_MODELS ?= 30

all : $(ALL)

include target/$(TARGET)/Makefile.inc

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

ODIR     = objs/$(TARGET)$(ODIREXT)

SRC_C    := $(SRC_C) $(wildcard *.c) $(wildcard screen/*.c) $(wildcard screen/$(SCREENSIZE)/*.c) \
                $(wildcard gui/*.c) $(wildcard pages/$(SCREENSIZE)/*.c) $(wildcard config/*.c)
ifdef MODULAR
SRC_C    := $(SRC_C) protocol/protocol.c
CFLAGS := $(CFLAGS) -DMODULAR=$(MODULAR)
PROTO_SRC_C := $(filter-out protocol/protocol.c, $(wildcard protocol/*.c))
PROTO_OBJS  := $(addprefix $(ODIR)/, $(notdir $(PROTO_SRC_C:.c=.o)))
PROTO_LIBS  := $(addprefix $(ODIR)/, $(notdir $(PROTO_SRC_C:.c=.bin)))
PROTO_EXTRA_OBJS := $(addprefix $(ODIR)/, $(notdir $(PROTO_EXTRA_C:.c=.o)))
SYMBOL_FILE := $(ODIR)/symbolfile
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

OBJS 	 = $(addprefix $(ODIR)/, $(notdir $(SRC_C:.c=.o) $(SRC_S:.s=.o) $(SRC_CXX:.cpp=.o)))

FONTS    = filesystem/devo10/media/12normal.fon \
           filesystem/devo8/media/15normal.fon \
           filesystem/devo8/media/23bold.fon

MODELDIR = filesystem/$(FILESYSTEM)/models
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
HGTEST := $(shell [ -f $(ODIR)/hgstamp ] || touch $(ODIR)/hgstamp; echo $(HGVERSION) | cmp -s $(ODIR)/hgstamp - || echo $(HGVERSION) > $(ODIR)/hgstamp)

ifeq "$(TYPE)" "dev"
  CFLAGS   := $(CFLAGS) -g -DBUILDTYPE_DEV
endif

CFLAGS   := $(CFLAGS) -Wall -Wextra -I. -D$(PROGMODE) -std=gnu99 -Itarget/$(TARGET) -Igui/$(SCREENSIZE) -DHGVERSION="\"${HGVERSION}\""
CXXFLAGS := $(CXXFLAGS) $(patsubst -std=gnu99,,$(CFLAGS))
EXEEXT   ?= elf

LAST_MODEL := $(MODELDIR)/model$(NUM_MODELS).ini

ALL += $(LAST_MODEL)

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
ifdef MODULAR
	$(NM) $@ | perl -ne 'if(! /[.\$$]/ && /^(\S+).* (\S+)$$/) {print "$$2 = 0x$$1;\n";}' > $(SYMBOL_FILE)
endif

#If any dependencies don't exist (like protocol.ld), this rule will fail with something like:
#make: *** No rule to make target `objs/devo7e/devo.bin', needed by `Makefile'.  Stop.
$(ODIR)/%.bin: $(ODIR)/%.o $(PROTO_LINKFILE) $(PROTO_EXTRA_OBJS) $(TARGET).$(EXEEXT)
	@echo " + Building '$@'"
	$(LD) -o $(@:.bin=.elf) $< $(PROTO_EXTRA_OBJS) -gc-sections -Map=$(@:.bin=.map) --cref -T$(PROTO_LINKFILE) --just-symbols=$(SYMBOL_FILE)
	$(CP) -O binary  $(@:.bin=.elf) $@

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

# The following enables quiet output unless you use VERBOSE=1
# Note that this must be after the 1st rule so that it doesn't execute by default
$(VERBOSE).SILENT:

clean:
	rm -f $(TARGET).$(EXEEXT) $(TARGET).bin $(TARGET).dfu $(TARGET).list $(TARGET).map $(ODIR)/*.o $(ODIR)/*.P

distclean:
	rm -f *.$(EXEEXT) *.bin *.dfu *.list *.map
	rm -rf objs
	make -C libopencm3 clean > /dev/null
	rm -rf deviation*.zip

flash: devo8.dfu
	dfu-util -a 0 -D devo8.dfu

language:
	../utils/extract_strings.pl -update

$(OBJS): | $(ODIR)

$(ODIR):
	@mkdir -p $@

$(MODELDIR):
	@mkdir -p $@

#Fonts
filesystem/devo10/media/12normal.fon: fonts/12normal.bdf
	../utils/bdf_to_font.pl -maxsize 12 -mode bin $< -out $@ -minspace 10

filesystem/devo8/media/15normal.fon: fonts/15normal.bdf
	../utils/bdf_to_font.pl -maxsize 15 -mode bin $< -out $@ -minspace 8

filesystem/devo8/media/23bold.fon: fonts/23bold.bdf
	../utils/bdf_to_font.pl -maxsize 23 -mode bin $< -out $@ -minspace 8

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

# Rebuild on hg version change.
$(ODIR)/version.o: $(ODIR)/hgstamp

$(ODIR)/hgstamp:
	echo $(HGVERSION) > $(ODIR)/hgstamp

# phony targets
.PHONY: clean
# recompile if the Makefile changes
$(OBJS) $(PROTO_OBJS) ($PROTO_EXTRA_OBJS): target/$(TARGET)/Makefile.inc $(EXTRA_MAKEFILES)

zip: $(TARGET).dfu
	mv -f $(TARGET).dfu deviation-$(HGVERSION).dfu
	rm -f deviation-$(HGVERSION).zip
	# make zipping more compatible with other distributions
	zip deviation-$(HGVERSION).zip deviation-$(HGVERSION).dfu
	rm -f deviation-$(HGVERSION).dfu

fs:
	rm -f deviation-fs-$(HGVERSION).zip; zip -r deviation-fs-$(HGVERSION).zip filesystem/$(TARGET)

release:
	make TARGET=devo8 TYPE=prd zip
	make TARGET=devo6 TYPE=prd zip
	make TARGET=devo10 TYPE=prd zip
	make TARGET=devo8 fs
	make TARGET=devo10 fs
	#make TARGET=emu_devo8 TYPE=prd WINDOWS=1 

#The following forces re-reading the Makefile so that the 'all' prerequisites can be defined
#after the definition of the 'all' rule
Makefile: $(ALL)
