#ifndef _SIMPLE_H_
#define _SIMPLE_H_
#include "../../common/standard/common_standard.h"

u8 SIMPLEMIX_ScrollCB(guiObject_t *parent, u8 pos, s8 direction, void *data);
#define ENTRIES_PER_PAGE (8 > NUM_CHANNELS ? NUM_CHANNELS : 8)

#endif
