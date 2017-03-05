ifndef BUILD_TARGET

SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp) $(wildcard target/common/emu/*.cpp)
SRC_C  = $(wildcard target/$(TARGET)/*.c) $(wildcard target/common/emu/*.c) \
         $(wildcard $(SDIR)/target/common/filesystems/*.c)

ifdef USE_INTERNAL_FS
SRC_C  += $(wildcard $(SDIR)/target/common/filesystems/devofs/*.c) \
          $(wildcard $(SDIR)/target/common/filesystems/petit_fat/*.c)
CFLAGS = -DEMULATOR=USE_INTERNAL_FS
else
CFLAGS = -DEMULATOR=USE_NATIVE_FS
endif

CFLAGS += -Itarget/common/emu -I$(SDIR)/target/common/filesystems
LFLAGS = -lfltk

ALL = $(TARGET).$(EXEEXT)

TYPE ?= dev

ifdef WINDOWS
    #CROSS=i586-mingw32msvc-
    ifdef CROSS
        FLTK_DIR      ?= /opt/fltk-w32
        PORTAUDIO_DIR ?= /opt/portaudio-w32
		MPG123_DIR ?= /opt/mpg123-w32
        LFLAGS += -mno-cygwin
        CFLAGS += -mno-cygwin
    else
        FLTK_DIR ?= /usr/local
        PORTAUDIO_DIR ?= /usr/local
        LFLAGS += -static-libgcc -static-libstdc++
    endif
    LFLAGS := -L$(FLTK_DIR)/lib -L$(PORTAUDIO_DIR)/lib $(LFLAGS) -lole32 -luuid -lcomctl32 -lwsock32 -lgdi32 -lcomdlg32 -lportaudio
    CFLAGS += -I$(FLTK_DIR)/include -I$(PORTAUDIO_DIR)/include -DWIN32
    EXEEXT = exe
    ODIREXT = -w32
else
    LFLAGS := $(LFLAGS) `fltk-config --cxxflags --ldflags` -lrt
    ifndef SOUND
        CFLAGS += -DNO_SOUND
    else
        LFLAGS := $(LFLAGS) -lportaudio
    endif
endif
else #BUILD_TARGET
CFLAGS += -DFILESYSTEM_DIR="\"filesystem/$(FILESYSTEM)\""

$(TARGET).zip: $(TARGET).$(EXEEXT) $(TARGET).fs_wrapper
	mv -f $(TARGET).$(EXEEXT) deviation-$(HGVERSION).$(EXEEXT)
	rm -f deviation-$(HGVERSION).zip
	zip deviation-$(HGVERSION).zip deviation-$(HGVERSION).$(EXEEXT)
ifdef WINDOWS
	zip -uj deviation-$(HGVERSION).zip $(PORTAUDIO_DIR)/bin/libportaudio-2.dll ; zip -u deviation-$(HGVERSION).zip $(MPG123_DIR)/bin/mpg123.exe
endif
ifeq "$(INCLUDE_FS)" "1"
	zip -ur deviation-$(HGVERSION).zip filesystem/$(FILESYSTEM)
endif
	rm -f deviation-$(HGVERSION).$(EXEEXT)

endif #BUILD_TARGET
