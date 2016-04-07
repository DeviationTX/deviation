
ifndef BUILD_TARGET
ALL += $(ODIR)/devo.fs

DEFAULT_PROTOCOLS = devo.mod dsm2.mod

REQUIRED_PROTOCOLS = $(addprefix $(ODIR)/protocol/, $(DEFAULT_PROTOCOLS))

else

$(ODIR)/devo.fs: $(wildcard filesystem/$(FILESYSTEM)/media/*.*) $(TARGET).dfu $(PRE_FS) $(TARGET).fs_wrapper $(REQUIRED_PROTOCOLS)
	rm -rf $(ODIR)/tmpfs | true
	cp -prf filesystem/$(FILESYSTEM) $(ODIR)/tmpfs
ifneq "$(REQUIRED_PROTOCOLS)" ""
	cp -pf $(REQUIRED_PROTOCOLS) $(ODIR)/tmpfs/protocol/
endif
	target/common/devo/devofs/buildfs.pl $(ODIR)/tmpfs > $@

endif
