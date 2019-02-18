SCREENSIZE  := 320x240x16
FILESYSTEMS := common base_fonts 320x240x16
FONTS        = filesystem/$(FILESYSTEM)/media/15normal.fon \
               filesystem/$(FILESYSTEM)/media/23bold.fon
LANGUAGE    := devo8

CFLAGS += -DTEST --coverage -g -O0 -fPIC
ifndef BUILD_TARGET

SRC_C  = $(wildcard $(SDIR)/target/tx/$(FAMILY)/$(TARGET)/*.c) \
         $(wildcard $(SDIR)/target/drivers/filesystems/*.c)

ifdef USE_INTERNAL_FS
SRC_C  += $(wildcard $(SDIR)/target/drivers/filesystems/devofs/*.c) \
          $(wildcard $(SDIR)/target/drivers/filesystems/petit_fat/*.c)
CFLAGS = -DEMULATOR=USE_INTERNAL_FS
else
CFLAGS = -DEMULATOR=USE_NATIVE_FS
endif

CFLAGS += -I$(SDIR)/target/drivers/filesystems
LFLAGS += -lz

ALL = $(TARGET).$(EXEEXT)

TYPE ?= dev

SRC_C += objs/test/AllTests.c

else #BUILD_TARGET
CFLAGS += -DFILESYSTEM_DIR="\"filesystem/$(FILESYSTEM)\""

objs/test/AllTests.c: target/tx/$(FAMILY)/$(TARGET)/make-tests.sh tests/*.c
	sh target/tx/$(FAMILY)/$(TARGET)/make-tests.sh > $(ODIR)/AllTests.c

endif #BUILD_TARGET
