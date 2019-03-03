## the following disable builtin rules which we don't need
## and make debugging harder
unexport LANGUAGE
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

# Setup common folder structure variables
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
SDIR = $(dir $(mkfile_path))
export

###########################################
## Provide some convenient make targets  ##
###########################################
TARGET   ?= devo8
default: $(TARGET)

ALLTARGETS = $(filter-out common,$(notdir $(wildcard target/tx/*/*)))

# The supported transmitters
TXS ?= devo6 devo7e devo7e-256 devo8 devo10 devo12e devo12 devof7 devof7-XMS devof4 devof4-XMS devof12e devof12e-XMS at9 at10 t8sg t8sg_v2 t8sg_v2_plus ir8m

#Filter non-existant emus
EMUS = $(foreach dir,$(TXS:%=emu_%),$(if $(wildcard target/$(dir)),$(dir),))
ALLEMUS = $(foreach dir,$(ALLTXS:%=emu_%),$(if $(wildcard target/$(dir)),$(dir),))

txs: $(TXS)
emus: $(EMUS)
both: txs emus
fss: $(TXS:%=fs_%)
zips: $(TXS:%=zip_%)
emuzips: $(EMUS:%=zip_%)
winzips: $(EMUS:%=zip_win_%)
everything: txs emus fss

# Generate the targets for a transmitter.
define make-target
$1:
	+$(MAKE) -C target/tx/$2/$1
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
$(foreach t,$(ALLTARGETS),$(eval $(call make-target,$t,$(notdir $(patsubst %/,%,$(dir $(wildcard target/tx/*/$(TARGET))))))))

.PHONY: distclean
distclean:
	rm -f *.$(EXEEXT) *.exe *.bin *.dfu *.list *.map
	rm -rf objs
	rm -rf  filesystem
	$(MAKE) -C libopencm3 clean > /dev/null
	rm -rf deviation*.zip
