SCREENSIZE  := 320x240x16
FILESYSTEMS := common base_fonts 320x240x16
FONTS        = filesystem/$(FILESYSTEM)/media/15normal.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon
LANGUAGE    := devo8

TARGET = test
FAMILY = other

CFLAGS += -DTEST --coverage -g -O0 -fPIC
SRC_C  += $(wildcard $(SDIR)target/tx/$(FAMILY)/$(TARGET)/*.c) \
         $(wildcard $(SDIR)target/drivers/filesystems/*.c)

ifdef USE_INTERNAL_FS
SRC_C  += $(wildcard $(SDIR)target/drivers/filesystems/devofs/*.c) \
          $(wildcard $(SDIR)target/drivers/filesystems/petit_fat/*.c)
CFLAGS = -DEMULATOR=USE_INTERNAL_FS
else
CFLAGS = -DEMULATOR=USE_NATIVE_FS
endif

TYPE ?= dev

CFLAGS += -I$(SDIR)target/drivers/filesystems
LFLAGS += -lz

SRC_C += objs/test/AllTests.c

CFLAGS += -DFILESYSTEM_DIR="\"filesystem/$(FILESYSTEM)\""

EXEEXT =
ALL += $(ODIR)AllTests.c $(TARGET).$(EXEEXT)

default: $(ODIR)AllTests.c $(TARGET).$(EXEEXT)

$(ODIR)AllTests.c: make-tests.sh $(SDIR)tests/*.c
	echo $(PWD)
	sh make-tests.sh > $(ODIR)AllTests.c

include $(SDIR)Makefile.inc
