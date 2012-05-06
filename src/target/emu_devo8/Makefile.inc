SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp)
SRC_C  = $(wildcard target/$(TARGET)/*.c)

SCREENSIZE = qvga
LFLAGS = -lfltk
all = $(TARGET)

