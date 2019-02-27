
#include "target/drivers/mcu/emu/common_emu.h"
#include "../t8sg/target_defs.h"
#undef HAS_OLED_DISPLAY
char *getenv(const char *);
static inline int _HAS_OLED_DISPLAY() {
    return getenv("OLED") ? 1 : 0;
}
#define HAS_OLED_DISPLAY _HAS_OLED_DISPLAY()

#define BUTTON_MAP { 'A', 'Q', 'D', 'E', 'S', 'W', 'F', 'R', FL_Left, FL_Right, FL_Down, FL_Up, 13/*FL_Enter*/, FL_Escape, 0 }
