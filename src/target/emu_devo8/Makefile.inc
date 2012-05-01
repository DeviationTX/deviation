SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp)

SCREENSIZE = qvga
CFLAGS = -DHAS_FS -DTRANSPARENT_FONT
LFLAGS = -lfltk
all = $(TARGET)

