## the following disable builtin rules which we don't need
## and make debugging harder
unexport LANGUAGE
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

# Setup common folder structure variables
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
export SDIR = $(dir $(mkfile_path))

###########################################
## Provide some convenient make targets  ##
###########################################

ALLTARGETS = $(filter-out common,$(notdir $(wildcard target/tx/*/*)))

# The supported transmitters
TXS ?= devo6 devo7e devo7e-256 devo8 devo10 devo12e devo12 devof7 devof7-XMS devof4 devof4-XMS devof12e devof12e-XMS at9 at10 t8sg t8sg_v2 t8sg_v2_plus ir8m

#Filter non-existant emus
EMUS = $(addprefix emu_, $(TXS))
ALLEMUS = $(addprefix emu_, $(ALLTXS))

default:
	@echo specify the target to build. target can be one of
	@echo $(TXS)
	@echo $(addprefix emu_, $(TXS))

.PHONY: txs emus both fss zips emuzips winzips release everything
txs: $(TXS)
emus: $(EMUS)
both: txs emus
fss: $(TXS:%=fs_%)
zips: $(TXS:%=zip_%)
emuzips: $(EMUS:%=zip_%)
winzips: $(EMUS:%=zip_win_%)
release:  $(TXS:%=zip_%) $(EMUS:%=zip_win_%) $(TXS:%=fs_%)
everything: txs emus fss

# Generate the targets for a transmitter.
define make-target
.PHONY: $1 win_$1 fs_$1 both_$1 zip_$1 zip_win_$1 release_$1 all_$1
$1:
	+$(MAKE) -f target/tx/$2/$1/Makefile
win_$1:
ifeq ($(OS),Windows_NT)
	# This builds Windows emulators on a Windows system
	+$(MAKE) -C target/tx/$2/$1 WINDOWS=1
else
	# This builds Windows emulators on a Linux system
	+$(MAKE) -C target/tx/$2/$1 WINDOWS=1 CROSS=i586-mingw32msvc-
endif
fs_$1:
ifneq "$(INCLUDE_FS)" "1"
	+$(MAKE) -C target/tx/$2/$1 fs
endif
both_$1: $1 emu_$1
zip_$1:
	+$(MAKE) -C target/tx/$2/$1 zip
zip_win_$1:
ifeq ($(OS),Windows_NT)
	# This builds Windows emulators on a Windows system
	+$(MAKE) -C target/tx/$2/$1 WINDOWS=1 zip
else
	# This builds Windows emulators on a Linux system
	+$(MAKE) -C target/tx/$2/$1 WINDOWS=1 CROSS=i586-mingw32msvc- zip
endif
release_$1: zip_$1 zip_win_emu_$1 fs_$1
all_$1:
	+$(MAKE) -C target/tx/$2/$1 all
endef

# Now generate all the convenience targets
$(foreach t,$(ALLTARGETS),$(eval $(call make-target,$t,$(notdir $(patsubst %/,%,$(dir $(wildcard target/tx/*/$(t))))))))

.PHONY: distclean coverity lint
distclean:
	rm -f *.$(EXEEXT) *.exe *.bin *.dfu *.list *.map
	rm -rf objs
	rm -rf  filesystem
	$(MAKE) -C libopencm3 clean > /dev/null
	rm -rf deviation*.zip

coverity:
	rm -rf cov-int 2>/dev/null; $(MAKE) distclean
	cov-build --dir cov-int make -C target/tx/devo/devo8
	cov-build --dir cov-int make -C target/tx/devo/devo10
	tar -cvzf coverity-$(HGVERSION).tgz cov-int

lint:
	../utils/run_linter.py --diff --debug
