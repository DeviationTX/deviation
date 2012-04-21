SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp)

SCREENSIZE = qvga
CFLAGS = -DHELLO_WORLD
LFLAGS = -lfltk
all = $(TARGET)

