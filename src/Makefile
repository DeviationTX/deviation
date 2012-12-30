# Use VERBOSE=1 to enable verbose make
TYPE     ?= dev
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

SRC_C    := $(SRC_C) $(wildcard *.c) $(wildcard screen/*.c) $(wildcard screen/$(SCREENSIZE)/*.c) \
                $(wildcard gui/*.c) $(wildcard pages/$(SCREENSIZE)/*.c) $(wildcard protocol/*.c) \
                $(wildcard pages/$(SCREENSIZE)/advanced/*.c) \
                $(wildcard pages/$(SCREENSIZE)/simple/*.c) \
                $(wildcard config/*.c)
SRC_S    := $(SRC_S) $(wildcard *.s)
SRC_CXX  := $(SRC_CXX) $(wildcard *.cpp)
SRCS     := $(SRC_C) $(SRC_CXX) $(SRC_S)

ODIR     = objs/$(TARGET)$(ODIREXT)
OBJS 	 = $(addprefix $(ODIR)/, $(notdir $(SRC_C:.c=.o) $(SRC_S:.s=.o) $(SRC_CXX:.cpp=.o)))

FONTS    = filesystem/devo10/media/12normal.fon \
           filesystem/devo8/media/15normal.fon \
           filesystem/devo8/media/23bold.fon

MODELDIR = filesystem/models
ifeq "$(wildcard ../.hg )" ""
  HGVERSION := ${TARGET}-Unknown
else
  ifeq "$(shell hg parents --template {latesttagdistance})" "1"
    HGVERSION := $(shell hg parents --template '${TARGET}-{latesttag}')
  else
    HGVERSION := $(shell hg parents --template '${TARGET}-{node|short}')
  endif
endif

ifeq "$(TYPE)" "dev"
  CFLAGS   := $(CFLAGS) -g -DBUILDTYPE_DEV
else
  CFLAGS   := $(CFLAGS) -O2
endif

CFLAGS   := $(CFLAGS) -Wall -Wextra -I. -D$(PROGMODE) -std=gnu99 -Itarget/$(TARGET) -Igui/$(SCREENSIZE) -DHGVERSION="\"${HGVERSION}\""
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

$(LAST_MODEL): model_template.ini tx_template.ini $(FONTS)
	@echo " + Copying template files"
	for tx in devo8 devo10; do \
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
			done; \
		done

# The following enables quiet output unless you use VERBOSE=1
# Note that this must be after the 1st rule so that it doesn't execute by default
$(VERBOSE).SILENT:

clean:
	rm -f $(TARGET).$(EXEEXT) $(TARGET).bin $(TARGET).dfu $(TARGET).list $(TARGET).map $(ODIR)/*.o $(ODIR)/*.P

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

# Rebuild on hg version change.
# From http://mercurial.selenic.com/wiki/VersioningWithMake
$(ODIR)/version.o: $(ODIR)/hgstamp
.PHONY: $(ODIR)/hgstamp
$(ODIR)/hgstamp:
	[ -f $@ ] || touch $@
	echo $(HGVERSION) | cmp -s $@ - || echo $(HGVERSION) > $@

# phony targets
.PHONY: clean
# recompile if the Makefile changes
$(OBJS): Makefile target/$(TARGET)/Makefile.inc

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
