
ifndef BUILD_TARGET
ALL += $(ODIR)/devo.fs

DEFAULT_PROTOCOLS = devo.mod dsm2.mod

REQUIRED_PROTOCOLS = $(addprefix $(ODIR)/protocol/, $(DEFAULT_PROTOCOLS))

else

$(ODIR)/devo.fs: $(LAST_MODEL) $(TARGET).dfu $(PRE_FS) $(TARGET).fs_wrapper $(REQUIRED_PROTOCOLS)
	rm -rf $(ODIR)/tmpfs | true
	cp -prf filesystem/$(FILESYSTEM) $(ODIR)/tmpfs
	mkdir $(ODIR)/tmpfs/protocol || true
ifneq "$(REQUIRED_PROTOCOLS)" ""
	cp -pf $(REQUIRED_PROTOCOLS) $(ODIR)/tmpfs/protocol/
endif
	target/common/filesystems/devofs/buildfs.py -c -f $@ -d $(ODIR)/tmpfs

endif
