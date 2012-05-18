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
    LFLAGS := -L$(FLTK_DIR)/lib $(LFLAGS) -mwindows -lole32 -luuid -lcomctl32 -lwsock32
    CFLAGS := -I$(FLTK_DIR)/include -mwindows -DWIN32 $(CFLAGS)
    EXEEXT = exe
    ODIREXT = -w32
endif
