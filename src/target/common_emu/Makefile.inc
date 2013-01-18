ifndef BUILD_TARGET

SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp) $(wildcard target/common_emu/*.cpp)
SRC_C  = $(wildcard target/$(TARGET)/*.c) $(wildcard target/common_emu/*.c)

LFLAGS = -lfltk
CFLAGS = -DEMULATOR

ALL = $(TARGET).$(EXEEXT)

TYPE ?= dev

ifdef WINDOWS
    #CROSS=i586-mingw32msvc-
    FLTK_DIR = /usr/local
    ifdef CROSS
        FLTK_DIR = /opt/fltk-w32
        PORTAUDIO_DIR = /opt/portaudio-w32
        LFLAGS += -mno-cygwin
        CFLAGS += -mno-cygwin
    endif
    LFLAGS := -L$(FLTK_DIR)/lib -L$(PORTAUDIO_DIR)/lib $(LFLAGS) -lole32 -luuid -lcomctl32 -lwsock32 -lgdi32 -lcomdlg32 -lportaudio
    CFLAGS += -I$(FLTK_DIR)/include -I$(PORTAUDIO_DIR)/include -DWIN32
    EXEEXT = exe
    ODIREXT = -w32
else
    LFLAGS := $(LFLAGS) -lrt
    ifndef SOUND
        CFLAGS += -DNO_SOUND
    else
        LFLAGS := $(LFLAGS) -lportaudio
    endif
endif
endif #BUILD_TARGET
