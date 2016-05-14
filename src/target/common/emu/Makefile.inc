ifndef BUILD_TARGET

SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp) $(wildcard target/common/emu/*.cpp)
SRC_C  = $(wildcard target/$(TARGET)/*.c) $(wildcard target/common/emu/*.c)

LFLAGS = -lfltk
CFLAGS = -DEMULATOR -Itarget/common/emu

ALL = $(TARGET).$(EXEEXT)

TYPE ?= dev

ifdef WINDOWS
    #CROSS=i586-mingw32msvc-
    ifdef CROSS
        FLTK_DIR      ?= /opt/fltk-w32
        PORTAUDIO_DIR ?= /opt/portaudio-w32
        LFLAGS += -mno-cygwin
        CFLAGS += -mno-cygwin
    else
        FLTK_DIR ?= /usr/local
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

$(TARGET).zip: $(TARGET).$(EXEEXT)
	mv -f $(TARGET).$(EXEEXT) deviation-$(HGVERSION).$(EXEEXT)
	rm -f deviation-$(HGVERSION).zip
	zip deviation-$(HGVERSION).zip deviation-$(HGVERSION).$(EXEEXT)
ifdef WINDOWS
	zip -u deviation-$(HGVERSION).zip libportaudio-2.dll
endif
ifeq "$(INCLUDE_FS)" "1"
	zip -ur deviation-$(HGVERSION).zip filesystem/$(FILESYSTEM)
endif
	rm -f deviation-$(HGVERSION).$(EXEEXT)

endif #BUILD_TARGET
