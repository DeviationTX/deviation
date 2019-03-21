#################
#The filesystem #
#################

%.fs_wrapper: $(LAST_MODEL)
	true

.PHONY: $(PRE_FS) $(LAST_MODEL)
$(LAST_MODEL): $(SDIR)/model_template.ini $(SDIR)/tx_template.ini $(FONTS) $(PRE_FS)
	@echo " + Copying template files for $(FILESYSTEM)"
	mkdir -p $(FS_ROOT) || true
	for i in $(FILESYSTEMS); do cp -prf $(SDIR)/fs/$$i/* $(FS_ROOT)/; done
	rm $(FS_ROOT)/*/.ignore 2>/dev/null || true
	cp $(SDIR)/tx_template.ini $(FS_ROOT)/tx.ini
	echo 'empty' > $(FS_ROOT)/errors.txt
	mkdir $(FS_ROOT)/models 2> /dev/null || true
	echo 'name=Model1' > $(FS_ROOT)/models/model1.ini \
		&& cat $(SDIR)/model_template.ini >> $(FS_ROOT)/models/model1.ini
	cp $(SDIR)/model_template.ini $(FS_ROOT)/models/default.ini
ifdef LANGUAGE
	mkdir $(FS_ROOT)/language 2> /dev/null; \
               $(SDIR)/../utils/extract_strings.pl -po -fs $(FS_ROOT)/language -targets $(LANGUAGE) -update -objdir $(ODIR)
endif
	export tx=$(FILESYSTEM); \
	number=2 ; while [ $$number -le $(NUM_MODELS) ] ; do \
		cp $(SDIR)/model_template.ini filesystem/$$tx/models/model$$number.ini; \
		number=`expr $$number + 1`; \
		done
	@echo " + Checking string list length for $(FILESYSTEM)"
ifeq "$(TYPE)" "dev"
	$(SDIR)/../utils/check_string_size.pl -target $(FILESYSTEM) -objdir $(ODIR)
else
	$(SDIR)/../utils/check_string_size.pl -target $(FILESYSTEM) -objdir $(ODIR) -quiet
endif
	$(SDIR)/../utils/run_linter.py --diff --skip-github --no-fail

######################
#Necessary Font files#
######################
$(FS_ROOT)/media/12normal.fon: $(SDIR)/fonts/12normal.bdf
	mkdir -p $(FS_ROOT)/media/
	$(SDIR)/../utils/font/bdf_to_font.pl -maxsize 12 -mode bin $< -out $@ -minspace 10

$(FS_ROOT)/media/15normal.fon: $(SDIR)/fonts/15normal.bdf
	mkdir -p $(FS_ROOT)/media/
	$(SDIR)/../utils/font/bdf_to_font.pl -maxsize 15 -mode bin $< -out $@ -minspace 8

# special font for f12e with 18x12 char size
$(FS_ROOT)/media/f12e.fon: $(SDIR)/fonts/f12e.bdf
	mkdir -p $(FS_ROOT)/media/
	$(SDIR)/../utils/font/bdf_to_font.pl -maxsize 18 -mode bin $< -out $@ -minspace 8

$(FS_ROOT)/media/12ascii.fon: $(SDIR)/fonts/12normal.bdf
	mkdir -p $(FS_ROOT)/media/
	$(SDIR)/../utils/font/bdf_to_font.pl -maxsize 12 -mode bin $< -out $@ -minspace 10 -ascii

$(FS_ROOT)/media/15ascii.fon: $(SDIR)/fonts/15normal.bdf
	mkdir -p $(FS_ROOT)/media/
	$(SDIR)/../utils/font/bdf_to_font.pl -maxsize 15 -mode bin $< -out $@ -minspace 8 -ascii

$(FS_ROOT)/media/23bold.fon: $(SDIR)/fonts/23bold.bdf
	mkdir -p $(FS_ROOT)/media/
	$(SDIR)/../utils/font/bdf_to_font.pl -maxsize 23 -mode bin $< -out $@ -minspace 8

$(FS_ROOT)/media/04b03.fon: $(SDIR)/fonts/04b03.bdf
	mkdir -p $(FS_ROOT)/media/
	$(SDIR)/../utils/font/bdf_to_font.pl -maxsize 5 -mode bin $< -out $@ -minspace 5
