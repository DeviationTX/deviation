HAS_4IN1_FLASH ?= 0
HAS_FLASH_DETECT ?= $(HAS_4IN1_FLASH)

SCREENSIZE  := 320x240x16
FONTS        = filesystem/$(FILESYSTEM)/media/15ascii.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon

DFU_ARGS    := -D 0x0483:0xdf12 -b 0x08003000

include $(SDIR)/target/tx/radiolink/common/Makefile.inc

ifndef BUILD_TARGET

ifneq "$(HAS_4IN1_FLASH)" "1"
EXTRA_DFUS = $(TARGET)-fs.dfu
endif

ALL = $(LIBOPENCM3) $(TARGET).dfu $(EXTRA_DFUS)

SRC_C   += $(SDIR)/target/drivers/display/8080/320x240x16.c \
           $(wildcard $(SDIR)/target/drivers/input/rotary/*.c)

else #BUILD_TARGET

$(TARGET).dfu: $(TARGET).bin
	$(SDIR)/../utils/dfu.py --name "$(HGVERSION) Firmware" $(DFU_ARGS):$< $@
	$(SDIR)/../utils/get_mem_usage.pl $(TARGET).map

$(ODIR)/devo.fs: $(LAST_MODEL) $(TARGET).dfu $(PRE_FS) $(TARGET).fs_wrapper
	target/drivers/filesystems/devofs/buildfs.py -c -i -f $@ -d filesystem/$(FILESYSTEM)

$(TARGET)-fs.dfu: $(ODIR)/devo.fs
	$(SDIR)/../utils/dfu.py --name "$(HGVERSION) Filesystem" -D 0x0483:0xdf12 -b 0x08040000:$< $@

$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/=15normal/=15ascii/' filesystem/$(FILESYSTEM)/media/config.ini
	rm filesystem/$(FILESYSTEM)/datalog.bin

$(TARGET).zip: $(ALL)
	cp -f $(TARGET).dfu deviation-$(HGVERSION).dfu
	rm -f deviation-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	zip debug-$(HGVERSION).zip $(TARGET).elf
	# make zipping more compatible with other distributions
	zip deviation-$(HGVERSION).zip deviation-$(HGVERSION).dfu $(EXTRA_DFUS)
	zip -u deviation-$(HGVERSION).zip debug-$(HGVERSION).zip
	rm -f debug-$(HGVERSION).zip
	rm -f deviation-$(HGVERSION).dfu

endif #BUILD_TARGET
