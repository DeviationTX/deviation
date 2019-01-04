SCREENSIZE  := 320x240x16
FILESYSTEMS := common base_fonts 320x240x16
FONTS        = filesystem/$(FILESYSTEM)/media/15normal.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon
LANGUAGE    := devo8
CFLAGS += -DTEST --coverage -g
include target/common/emu/Makefile.inc

ifndef BUILD_TARGET
SRC_CXX =
SRC_C += objs/test/AllTests.c
else
objs/test/AllTests.c: target/test/make-tests.sh
	sh target/test/make-tests.sh > $(ODIR)/AllTests.c
endif
