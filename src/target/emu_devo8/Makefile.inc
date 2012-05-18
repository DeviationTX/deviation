SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp)
SRC_C  = $(wildcard target/$(TARGET)/*.c)

LFLAGS = -lfltk
SCREENSIZE = qvga
all = $(TARGET)

ifdef WINDOWS
    CROSS=i586-mingw32msvc-
    LFLAGS = -L/opt/fltk-w32/lib $(LFLAGS) -mwindows  -mno-cygwin  -lole32 -luuid -lcomctl32 -lwsock32
    CFLAGS = -I/opt/fltk-w32/include -mno-cygwin -mwindows -DWIN32  -mno-cygwin
    EXEEXT = exe
    ODIREXT = -w32
endif
