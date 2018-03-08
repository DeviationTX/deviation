#ifndef _STANDARD_H_
#define _STANDARD_H_
#include "../../common/standard/common_standard.h"

int STDMIX_ScrollCB(guiObject_t *parent, u8 pos, s8 direction, void *data);
#define ENTRIES_BY_SCREENSIZE (LCD_WIDTH == 480 ? 9 : 8) // 320x240: 8, 480x272: 9
#define ENTRIES_PER_PAGE      (ENTRIES_BY_SCREENSIZE > NUM_CHANNELS ? NUM_CHANNELS : ENTRIES_BY_SCREENSIZE)

#endif
