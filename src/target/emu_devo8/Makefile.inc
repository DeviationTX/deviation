SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp)
SRC_C  = $(wildcard target/$(TARGET)/*.c)

LFLAGS = -lfltk
SCREENSIZE = qvga
all = $(TARGET)

ifdef WINDOWS
    #CROSS=i586-mingw32msvc-
    FLTK_DIR = /usr/local
    ifdef CROSS
        FLTK_DIR = /opt/fltk-w32
        LFLAGS := $(LFLAGS) -mno-cygwin
        CFLAGS := $(CFLAGS) -mno-cygwin
    endif
    LFLAGS := -L$(FLTK_DIR)/lib $(LFLAGS) -lole32 -luuid -lcomctl32 -lwsock32 -lgdi32 -lcomdlg32 -lportaudio
    CFLAGS := -I$(FLTK_DIR)/include -DWIN32 $(CFLAGS)
    EXEEXT = exe
    ODIREXT = -w32
else
    ifndef SOUND
        CFLAGS := -DNO_SOUND
    else
        LFLAGS := $(LFLAGS) -lportaudio
    endif
endif
