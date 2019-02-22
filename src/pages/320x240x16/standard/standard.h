#ifndef _STANDARD_H_
#define _STANDARD_H_
#include "../../common/standard/common_standard.h"

int STDMIX_ScrollCB(guiObject_t *parent, u8 pos, s8 direction, void *data);
#define ENTRIES_BY_SCREENSIZE ((LCD_HEIGHT / 30) + 1)  // 240: 8, 272: 9, 320: 10
#define ENTRIES_PER_PAGE      (ENTRIES_BY_SCREENSIZE > NUM_CHANNELS ? NUM_CHANNELS : ENTRIES_BY_SCREENSIZE)

#endif
