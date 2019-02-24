HAS_4IN1_FLASH ?= 0

SCREENSIZE  := 320x240x16
FONTS        = filesystem/$(FILESYSTEM)/media/15ascii.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon

ifeq "$(HAS_4IN1_FLASH)" "1"
FILESYSTEMS := common base_fonts 320x240x16
else
FILESYSTEMS := common base_fonts 320x240x1
endif

include $(SDIR)/target/tx/radiolink/common/Makefile.inc

ifndef BUILD_TARGET

ALL = $(LIBOPENCM3) $(TARGET).bin $(TARGET).mac

SRC_C   += $(SDIR)/target/drivers/display/8080/320x240x16.c \
           $(wildcard $(SDIR)/target/drivers/input/rotary/*.c)

ifneq "$(HAS_4IN1_FLASH)" "1"
SRC_C := $(filter-out $(SDIR)/target/common/stm32/spi_flash.c, $(SRC_C))
endif

else #BUILD_TARGET
$(TARGET).mac: $(TARGET).bin
	cat $(SDIR)/target/tx/radiolink/at10/header.mac $(TARGET).bin > $@

$(TARGET).bin: $(TARGET).elf $(ODIR)/devo.fs
	$(CP) -O binary $(TARGET).elf $(ODIR)/$@
	$(DUMP) -S $(TARGET).elf > $(TARGET).list
	$(SDIR)/../utils/get_mem_usage.pl $(TARGET).map
ifneq "$(HAS_4IN1_FLASH)" "1"
	# padding firmware to 216KB
	truncate --size=216K $(ODIR)/$@
	cat $(ODIR)/$@ $(ODIR)/devo.fs > $@
endif

$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/=15normal/=15ascii/' filesystem/$(FILESYSTEM)/media/config.ini
	perl -p -i -e 's/drawn_background=0/drawn_background=1/' filesystem/$(FILESYSTEM)/media/config.ini
	rm filesystem/$(FILESYSTEM)/datalog.bin

$(TARGET).zip: $(ALL)
	cp -f $(TARGET).bin deviation-$(HGVERSION).bin
	cp -f $(TARGET).mac deviation-$(HGVERSION).mac
	rm -f deviation-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	zip debug-$(HGVERSION).zip $(TARGET).elf
	# make zipping more compatible with other distributions
	zip deviation-$(HGVERSION).zip deviation-$(HGVERSION).mac deviation-$(HGVERSION).bin $(EXTRA_DFUS)
	zip -u deviation-$(HGVERSION).zip debug-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip

endif #BUILD_TARGET
