## the following disable builtin rules which we don't need
## and make debugging harder
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

# Use VERBOSE=1 to enable verbose make
TARGET	   = universaltx
TYPE      ?= dev
LIBOPENCM3 = ../libopencm3/lib/libopencm3_stm32f0.a

###############################################
# Source Files                                #
###############################################
SRC_C = universaltx.c \
        utx_uart.c    \
        utx_power.c   \
        utx_pactl.c

SRC_C    := $(SRC_C) \
            $(wildcard ../protocol/*.c) \
            $(wildcard ../protocol/spi/*.c) \
            $(wildcard ../target/common/stm32/*.c)

EXCLUDE  := ../protocol/ppmout.c ../protocol/usbhid.c ../protocol/protocol.c ../protocol/spi/avr_program.c
SRC_C    := $(filter-out $(EXCLUDE),$(SRC_C))

###############################################
#This section defines binaries needed to build#
###############################################
    CROSS = arm-none-eabi-
    CC   = $(CROSS)gcc
    CXX  = $(CROSS)g++
    LD   = $(CROSS)ld
    AR   = $(CROSS)ar
    AS   = $(CROSS)as
    CP   = $(CROSS)objcopy
    DUMP = $(CROSS)objdump
    NM   = $(CROSS)nm
###############################################
#END SECTION                                  #
###############################################

############################################
#This section intermediate build files     #
############################################
ODIR	= ../objs/$(TARGET)
OBJS	= $(addprefix $(ODIR)/, $(notdir $(SRC_C:.c=.o) $(SRC_S:.s=.o)))

ifeq "$(wildcard ../../.hg )" ""
  HGVERSION := ${TARGET}-Unknown
else
  HGVERSION := $(shell ../../utils/get_version.pl ${TARGET})
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
CFLAGS = -DSTM32F0 -mthumb -mcpu=cortex-m0 -msoft-float \
         -Wall -Wextra -Werror=undef -std=gnu99 \
         -DHGVERSION="\"${HGVERSION}\"" \
         -I../libopencm3/include/ -Iinclude
LFLAGS = --static -nostartfiles -Tuniversaltx.ld -Wl,-Map=universaltx.map -Wl,--gc-sections -L../libopencm3/lib/ -L../libopencm3/lib/stm32/f0/ -lopencm3_stm32f0 -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group
ifeq "$(TYPE)" "dev"
  CFLAGS   := $(CFLAGS) -g -DBUILDTYPE_DEV
endif
##################################################
#END SECTION                                     #
##################################################

all: $(LIBOPENCM3) $(TARGET).dfu

####################################
# recompile if the Makefile changes#
####################################
$(OBJS) $(PROTO_OBJS) ($PROTO_EXTRA_OBJS): Makefile

##################################################################################
# The following enables quiet output unless you use VERBOSE=1                    #
# Note that this must be after the 1st rule so that it doesn't execute by default#
##################################################################################
$(VERBOSE).SILENT:

.PHONY: clean

##########################################
#Ensure necessray directories are created#
##########################################
$(OBJS): | $(ODIR)

$(ODIR):
	@mkdir -p $@

######################
#The main executable#
######################
$(TARGET).elf: $(LINKFILE) $(OBJS) $(LIBOPENCM3)
	@echo " + Building '$@'"
	$(CC) -o $@ $(OBJS) $(LIBOPENCM3) $(LFLAGS) $(CFLAGS)

$(TARGET).dfu: $(TARGET).elf
	$(CP) -O binary $(TARGET).elf $(TARGET).bin
	$(DUMP) -d $(TARGET).elf > $(TARGET).list
	../../utils/dfu.py -b 0x08000000:$(TARGET).bin $(TARGET).dfu
	rm -f $(TARGET).bin

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
$(foreach directory,$(sort $(dir $(SRC_C) $(PROTO_EXTRA_C))),$(eval $(call define_compile_rules,$(directory))))

##############################
#Ensure version is up to date#
##############################
# Rebuild on hg version change.
$(ODIR)/version.o: $(ODIR)/hgstamp

$(ODIR)/hgstamp:
	echo $(HGVERSION) > $(ODIR)/hgstamp

#universaltx.elf: universaltx.c

#	arm-none-eabi-gcc --static -nostartfiles -Tuniversaltx.ld -Wl,-Map=universaltx.map -Wl,--gc-sections -DSTM32F0 -mthumb -mcpu=cortex-m0 -msoft-float -o universaltx.elf -I../libopencm3/include/ -L../libopencm3/lib/ -L../libopencm3/lib/stm32/f0/ universaltx.c -lopencm3_stm32f0 -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

#universaltx.dfu: universaltx.elf
#	arm-none-eabi-objcopy -O binary universaltx.elf universaltx.bin
#	arm-none-eabi-objdump -d universaltx.elf > universaltx.list
#	../../utils/dfu.py -b 0x08000000:universaltx.bin universaltx.dfu
#	cp universaltx.dfu /media/sf_cdrive/Users/ghaushe/Desktop/dfu/

$(LIBOPENCM3):
	make -C ../libopencm3 TARGETS=stm32/f0 lib

clean:
	rm -f $(TARGET).elf $(TARGET).bin $(TARGET).dfu $(TARGET).list \
		$(TARGET).map $(ODIR)/*.o $(ODIR)/*.o_ $(ODIR)/*.P  $(ODIR)/*.bin \
		2> /dev/null || true
