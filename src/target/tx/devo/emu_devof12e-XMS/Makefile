SCREENSIZE  := text
FILESYSTEMS := common text text_gfx
LANGUAGE    := devo10
FONTS        = filesystem/$(FILESYSTEM)/media/f12e.fon

TARGET = emu_devof12e-XMS
FAMILY = devo

SRC_C += $(SDIR)target/tx/devo/devof12e/lcd.c \
		$(SDIR)target/tx/devo/devof12e/mapped_gfx.c \
		$(SDIR)target/tx/devo/emu_devof12e/channels.c \
		$(SDIR)target/tx/devo/emu_devof12e/tw8816.c \
		$(SDIR)target/tx/devo/emu_devof12e/tw8816_map.c

CFLAGS += -I$(SDIR)target/tx/devo/emu_devof12e

include $(SDIR)target/drivers/mcu/emu/Makefile.inc
include $(SDIR)Makefile.inc

$(TARGET).fs_wrapper: $(LAST_MODEL)
	perl -p -i -e 's/=12normal/=f12e/' filesystem/$(FILESYSTEM)/media/config.ini

