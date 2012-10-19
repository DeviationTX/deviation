SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp) $(wildcard target/common_emu/*.cpp)
SRC_C  = $(wildcard target/$(TARGET)/*.c) $(wildcard target/common_emu/*.c)

LFLAGS = -lfltk
SCREENSIZE = 128x64x1
all = $(TARGET)
CFLAGS := -DEMULATOR

ifdef WINDOWS
    #CROSS=i586-mingw32msvc-
    FLTK_DIR = /usr/local
    ifdef CROSS
        FLTK_DIR = /opt/fltk-w32
        PORTAUDIO_DIR = /opt/portaudio-w32
        LFLAGS := $(LFLAGS) -mno-cygwin
        CFLAGS := $(CFLAGS) -mno-cygwin
    endif
    LFLAGS := -L$(FLTK_DIR)/lib -L$(PORTAUDIO_DIR)/lib $(LFLAGS) -lole32 -luuid -lcomctl32 -lwsock32 -lgdi32 -lcomdlg32 -lportaudio
    CFLAGS := -I$(FLTK_DIR)/include -I$(PORTAUDIO_DIR)/include -DWIN32 $(CFLAGS)
    EXEEXT = exe
    ODIREXT = -w32
else
    ifndef SOUND
        CFLAGS := -DNO_SOUND $(CFLAGS)
    else
        LFLAGS := $(LFLAGS) -lportaudio
    endif
endif
