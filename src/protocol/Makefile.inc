ifndef BUILD_TARGET

PROTO_MODULES += $(ODIR)/protocol/devo.mod
PROTO_MODULES += $(ODIR)/protocol/dsm2.mod
PROTO_MODULES += $(ODIR)/protocol/dsmx.mod
PROTO_MODULES += $(ODIR)/protocol/flysky.mod
PROTO_MODULES += $(ODIR)/protocol/hubsan4.mod
PROTO_MODULES += $(ODIR)/protocol/j6pro.mod
PROTO_MODULES += $(ODIR)/protocol/ppm.mod
PROTO_MODULES += $(ODIR)/protocol/wk2801.mod
PROTO_MODULES += $(ODIR)/protocol/wk2601.mod
PROTO_MODULES += $(ODIR)/protocol/wk2401.mod
#PROTO_MODULES += $(ODIR)/protocol/frsky-1.mod
#PROTO_MODULES += $(ODIR)/protocol/frsky-2.mod
PROTO_MODULES += $(ODIR)/protocol/skyartec.mod
PROTO_MODULES += $(ODIR)/protocol/v202.mod
PROTO_MODULES += $(ODIR)/protocol/slt.mod
ALL += $(PROTO_MODULES)
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

$(ODIR)/protocol/frsky-1.mod : $(ODIR)/frsky1way_cc2500.bin
	@echo Building 'frsky1way' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/frsky-2.mod : $(ODIR)/frsky2way_cc2500.bin
	@echo Building 'frsky2way' module
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

$(ODIR)/protocol/skyartec.mod : $(ODIR)/skyartec_cc2500.bin
	@echo Building 'skyartec' module
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

$(ODIR)/protocol/v202.mod : $(ODIR)/v202_nrf24l01.bin
	@echo Building 'v202' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/slt.mod : $(ODIR)/slt_nrf24l01.bin
	@echo Building 'slt' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

endif #BUILD_TARGET
