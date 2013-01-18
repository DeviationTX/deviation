ifndef BUILD_TARGET

ALL += $(ODIR)/protocol/devo.mod
ALL += $(ODIR)/protocol/dsm2.mod
ALL += $(ODIR)/protocol/dsmx.mod
ALL += $(ODIR)/protocol/flysky.mod
ALL += $(ODIR)/protocol/hubsan4.mod
ALL += $(ODIR)/protocol/j6pro.mod
ALL += $(ODIR)/protocol/ppm.mod
ALL += $(ODIR)/protocol/wk2801.mod
ALL += $(ODIR)/protocol/wk2601.mod
ALL += $(ODIR)/protocol/wk2401.mod
else #BUILD_TARGET
$(ODIR)/protocol/devo.mod : $(ODIR)/devo.bin
	@echo Building 'devo' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/dsm2.mod : $(ODIR)/dsm2.bin
	@echo Building 'dsm2' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/dsmx.mod : $(ODIR)/dsm2.bin
	@echo Building 'dsmx' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/flysky.mod : $(ODIR)/flysky_a7105.bin
	@echo Building 'flysky' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/hubsan4.mod : $(ODIR)/hubsan_a7105.bin
	@echo Building 'hubsan4' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/j6pro.mod : $(ODIR)/j6pro.bin
	@echo Building 'j6pro' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/ppm.mod : $(ODIR)/ppmout.bin
	@echo Building 'ppm' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/wk2801.mod : $(ODIR)/wk2x01.bin
	@echo Building 'wk2801' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/wk2601.mod : $(ODIR)/wk2x01.bin
	@echo Building 'wk2601' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/wk2401.mod : $(ODIR)/wk2x01.bin
	@echo Building 'wk2401' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

endif #BUILD_TARGET
