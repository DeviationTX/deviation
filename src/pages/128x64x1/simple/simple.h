#ifndef _SIMPLE_H_
#define _SIMPLE_H_
#define VIEW_ID 0
#include "../../common/simple/common_simple.h"
struct page_defs {
    const char *title;
    const char *(*value)(guiObject_t *obj, int dir, void *data);
    void (*tgl)(guiObject_t *obj, void *data);
};
void SIMPLE_Init(const struct page_defs *page_defs);
#endif
