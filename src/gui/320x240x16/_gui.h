#ifndef _GUI_H_
#define _GUI_H_
#include "target.h"

#ifndef LCD_WIDTH
#define LCD_WIDTH 320
#endif

#ifndef LCD_HEIGHT
#define LCD_HEIGHT 240
#endif

#define LCD_DEPTH 16

enum ImageNames {
    FILE_BTN96_24,
    FILE_BTN48_24,
    FILE_BTN96_16,
    FILE_BTN64_16,
    FILE_BTN48_16,
    FILE_BTN32_16,
    FILE_SPIN192,
    FILE_SPIN96,
    FILE_SPIN64,
    FILE_SPIN32,
    FILE_SPINPRESS96,
    FILE_SPINPRESS64,
    FILE_SPINPRESS32,
    FILE_ARROW_16_UP,
    FILE_ARROW_16_DOWN,
    FILE_ARROW_16_RIGHT,
    FILE_ARROW_16_LEFT,
};
#define IMAGE_MAP_END (FILE_ARROW_16_LEFT + 1)

enum ButtonType {
    BUTTON_DEVO10,
    BUTTON_96,
    BUTTON_48,
    BUTTON_96x16,
    BUTTON_64x16,
    BUTTON_48x16,
    BUTTON_32x16,
};

#define DIALOG_BUTTON BUTTON_96

#define ARROW_LEFT  (&image_map[FILE_ARROW_16_LEFT])
#define ARROW_RIGHT (&image_map[FILE_ARROW_16_RIGHT])
#define ARROW_UP    (&image_map[FILE_ARROW_16_UP])
#define ARROW_DOWN  (&image_map[FILE_ARROW_16_DOWN])
#define ARROW_WIDTH 16
#define ARROW_HEIGHT 16
#define LINE_SPACING 2
#endif
