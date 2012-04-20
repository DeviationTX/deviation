SRC_C    = common/lcd_qvga.c
SRC_CXX  = $(wildcard target/$(TARGET)/*.cpp)

LFLAGS = -lfltk
all = $(TARGET)

