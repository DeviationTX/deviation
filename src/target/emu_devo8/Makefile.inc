SRC_C    = common/lcd_qvga.c
SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp)

CFLAGS = -DHELLO_WORLD
LFLAGS = -lfltk
all = $(TARGET)

