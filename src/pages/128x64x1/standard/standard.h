#ifndef _STANDARD_H_
#define _STANDARD_H_
#define VIEW_ID 0
#include "../../common/standard/common_standard.h"
struct page_defs {
    const char *title;
    const char *(*value)(guiObject_t *obj, int dir, void *data);
    void (*tgl)(guiObject_t *obj, void *data);
};
void STANDARD_Init(const struct page_defs *page_defs);
void STANDARD_DrawCurvePoints(guiLabel_t vallbl[], guiTextSelect_t val[],
        guiButton_t *auto_button_ptr, u8 selectable_bitmap,
        void (*auto_generate_cb)(struct guiObject *obj, const void *data),
        void (*press_cb)(guiObject_t *obj, void *data),
        const char *(*set_pointval_cb)(guiObject_t *obj, int value, void *data));
#endif
