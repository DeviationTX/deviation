SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp)

SCREENSIZE = qvga
CFLAGS = -DHAS_FS
LFLAGS = -lfltk
all = $(TARGET)

