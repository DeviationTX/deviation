#ifndef _MAIN_CONFIG_H_
#define _MAIN_CONFIG_H_

enum MainWidget {
    TRIM1,
    TRIM2,
    TRIM3,
    TRIM4,
    TRIM5,
    TRIM6,
    TOGGLE1,
    TOGGLE2,
    TOGGLE3,
    TOGGLE4,
    TOGGLE5,
    BOX1,
    BOX2,
    BOX3,
    BOX4,
    BOX5,
    BOX6,
    BOX7,
    BOX8,
    MODEL_ICO,
    BAR1,
    BAR2,
    BAR3,
    BAR4,
    BAR5,
    BAR6,
    BAR7,
    BAR8,
};

u8 MAINPAGE_GetWidgetLoc(enum MainWidget widget, u16 *x, u16 *y, u16 *w, u16 *h);

#endif
