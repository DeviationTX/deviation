FILESYSTEMS := common text text_gfx
SCREENSIZE  := text
DFU_ARGS    := -c 12 -b 0x08004000
LANGUAGE   := devo10

include $(SDIR)/target/common/devo/Makefile.inc


ifndef BUILD_TARGET

SRC_C   += $(SDIR)/target/devof12e/backlight.c \
           $(SDIR)/target/devof12e/channels.c \
           $(SDIR)/target/devof12e/crc.c \
           $(SDIR)/target/devof12e/lcd.c \
           $(SDIR)/target/devof12e/mapped_gfx.c \
           $(SDIR)/target/devof12e/tw8816.c \
           $(SDIR)/target/devof12e/tw8816_map.c \
           $(SDIR)/target/devof12e/tx_buttons.c

else

$(TARGET).fs_wrapper: $(LAST_MODEL)
	rm -f filesystem/$(FILESYSTEM)/language/lang.cn
	rm -f filesystem/$(FILESYSTEM)/language/lang.ru
	rm -f filesystem/$(FILESYSTEM)/language/lang.tw

endif
