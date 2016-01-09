ifndef BUILD_TARGET

PROTO_MODULES += $(ODIR)/protocol/devo.mod
PROTO_MODULES += $(ODIR)/protocol/dsm2.mod
PROTO_MODULES += $(ODIR)/protocol/dsmx.mod
PROTO_MODULES += $(ODIR)/protocol/flysky.mod
PROTO_MODULES += $(ODIR)/protocol/hubsan4.mod
PROTO_MODULES += $(ODIR)/protocol/joysway.mod
PROTO_MODULES += $(ODIR)/protocol/j6pro.mod
PROTO_MODULES += $(ODIR)/protocol/ppm.mod
PROTO_MODULES += $(ODIR)/protocol/wk2801.mod
PROTO_MODULES += $(ODIR)/protocol/wk2601.mod
PROTO_MODULES += $(ODIR)/protocol/wk2401.mod
PROTO_MODULES += $(ODIR)/protocol/frsky-v8.mod
PROTO_MODULES += $(ODIR)/protocol/frsky.mod
PROTO_MODULES += $(ODIR)/protocol/skyartec.mod
PROTO_MODULES += $(ODIR)/protocol/v202.mod
PROTO_MODULES += $(ODIR)/protocol/slt.mod
PROTO_MODULES += $(ODIR)/protocol/usbhid.mod
PROTO_MODULES += $(ODIR)/protocol/hisky.mod
PROTO_MODULES += $(ODIR)/protocol/yd717.mod
PROTO_MODULES += $(ODIR)/protocol/symax.mod
PROTO_MODULES += $(ODIR)/protocol/cflie.mod
PROTO_MODULES += $(ODIR)/protocol/h377.mod
PROTO_MODULES += $(ODIR)/protocol/hm830.mod
PROTO_MODULES += $(ODIR)/protocol/kn.mod
PROTO_MODULES += $(ODIR)/protocol/esky150.mod
PROTO_MODULES += $(ODIR)/protocol/esky.mod
PROTO_MODULES += $(ODIR)/protocol/bluefly.mod
PROTO_MODULES += $(ODIR)/protocol/cx10.mod
PROTO_MODULES += $(ODIR)/protocol/cg023.mod
PROTO_MODULES += $(ODIR)/protocol/hontai.mod
PROTO_MODULES += $(ODIR)/protocol/mt99xx.mod
PROTO_MODULES += $(ODIR)/protocol/fy326.mod
PROTO_MODULES += $(ODIR)/protocol/bayang.mod
PROTO_MODULES += $(ODIR)/protocol/h8-3d.mod

ALL += $(PROTO_MODULES)
else #BUILD_TARGET
$(ODIR)/protocol/devo.mod : $(ODIR)/devo_cyrf6936.bin
	@echo Building 'devo' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/dsm2.mod : $(ODIR)/dsm2_cyrf6936.bin
	@echo Building 'dsm2' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/dsmx.mod : $(ODIR)/dsm2_cyrf6936.bin
	@echo Building 'dsmx' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/flysky.mod : $(ODIR)/flysky_a7105.bin
	@echo Building 'flysky' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/frsky-v8.mod : $(ODIR)/frsky1way_cc2500.bin
	@echo Building 'frsky1way' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/frsky.mod : $(ODIR)/frsky2way_cc2500.bin
	@echo Building 'frsky2way' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/hubsan4.mod : $(ODIR)/hubsan_a7105.bin
	@echo Building 'hubsan4' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/joysway.mod : $(ODIR)/joysway_a7105.bin
	@echo Building 'joysway' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/j6pro.mod : $(ODIR)/j6pro_cyrf6936.bin
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
	@echo Building 'tacticsl' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/usbhid.mod : $(ODIR)/usbhid.bin
	@echo Building 'usbhid' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/hisky.mod : $(ODIR)/hisky_nrf24l01.bin
	@echo Building 'hisky' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/yd717.mod : $(ODIR)/yd717_nrf24l01.bin
	@echo Building 'yd717' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/symax.mod : $(ODIR)/symax_nrf24l01.bin
	@echo Building 'symax' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/cflie.mod : $(ODIR)/cflie_nrf24l01.bin
	@echo Building 'cflie' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/h377.mod : $(ODIR)/h377_nrf24l01.bin
	@echo Building 'h377' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/hm830.mod : $(ODIR)/hm830_nrf24l01.bin
	@echo Building 'hm830' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/kn.mod : $(ODIR)/kn_nrf24l01.bin
	@echo Building 'kn' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/esky150.mod : $(ODIR)/esky150_nrf24l01.bin
	@echo Building 'esky150' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/esky.mod : $(ODIR)/esky_nrf24l01.bin
	@echo Building 'esky' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/bluefly.mod : $(ODIR)/bluefly_nrf24l01.bin
	@echo Building 'bluefly' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/cx10.mod : $(ODIR)/cx10_nrf24l01.bin
	@echo Building 'cx10' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/cg023.mod : $(ODIR)/cg023_nrf24l01.bin
	@echo Building 'cg023' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@    
    
$(ODIR)/protocol/hontai.mod : $(ODIR)/hontai_nrf24l01.bin
	@echo Building 'hontai' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@    

$(ODIR)/protocol/mt99xx.mod : $(ODIR)/mt99xx_nrf24l01.bin
	@echo Building 'mt99xx' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@

$(ODIR)/protocol/fy326.mod : $(ODIR)/fy326_nrf24l01.bin
	@echo Building 'fy326' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@    

$(ODIR)/protocol/bayang.mod : $(ODIR)/bayang_nrf24l01.bin
	@echo Building 'bayang' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@    

$(ODIR)/protocol/h8-3d.mod : $(ODIR)/h8_3d_nrf24l01.bin
	@echo Building 'h8-3d' module
	/bin/mkdir -p $(ODIR)/protocol/ 2>/dev/null; /bin/cp $< $@    
    
endif #BUILD_TARGET
