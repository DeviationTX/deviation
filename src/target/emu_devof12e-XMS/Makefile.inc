SCREENSIZE  := text
FILESYSTEMS := common text text_gfx
LANGUAGE    := devo10


include target/common/emu/Makefile.inc


ifdef BUILD_TARGET
$(TARGET).fs_wrapper: $(LAST_MODEL)
	rm filesystem/$(FILESYSTEM)/language/lang.cn
	rm filesystem/$(FILESYSTEM)/language/lang.ru
	rm filesystem/$(FILESYSTEM)/language/lang.tw
endif
