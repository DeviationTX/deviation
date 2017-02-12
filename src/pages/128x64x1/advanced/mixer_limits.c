/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "../pages.h"
#include <stdlib.h>
enum {
    TITLE_X  = 0,
    TITLE_W  = LCD_WIDTH - 51,
    REVERT_X = LCD_WIDTH - 50,
    REVERT_W = 50,
    LABEL_X  = 0,
    LABEL_W  = 60,
    TEXTSEL_X = 60,
    TEXTSEL_W = 60,
};
#endif //OVERRIDE_PLACEMENT

#include "../../common/advanced/_mixer_limits.c"

static unsigned action_cb(u32 button, unsigned flags, void *data);
static void revert_cb(guiObject_t *obj, const void *data);

static void _show_titlerow()
{
    (void)okcancel_cb;
    PAGE_SetActionCB(action_cb);
    mp->entries_per_page = 4;
    memset(gui, 0, sizeof(*gui));

    labelDesc.style = LABEL_UNDERLINE;
    GUI_CreateLabelBox(&gui->title, TITLE_X, 0 , TITLE_W, HEADER_HEIGHT, &labelDesc,
            MIXPAGE_ChanNameProtoCB, NULL, (void *)(long)mp->channel);
    labelDesc.style = LABEL_CENTER;
    GUI_CreateButtonPlateText(&gui->revert, REVERT_X, 0, REVERT_W, HEADER_WIDGET_HEIGHT, &labelDesc, NULL, revert_cb, (void *)_tr("Revert"));
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    (void)col;
    return (guiObject_t *)&gui->value[relrow];
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    void * tgl = NULL;
    void * label_cb = NULL;
    const void * label = NULL;
    void * disp = NULL;
    void * input_disp = NULL;
    void * value = NULL;
    switch(absrow) {
        case ITEM_REVERSE:
            label = _tr("Reverse");
            tgl = toggle_reverse_cb; disp = reverse_cb; value = (void *)((long)mp->channel);
            break;
        case ITEM_FAILSAFE:
            label = _tr("Fail-safe");
            tgl = toggle_failsafe_cb; disp = set_failsafe_cb;
            break;
        case ITEM_SAFETY:
            label = _tr("Safety");
            tgl = sourceselect_cb; disp = set_source_cb; value = &mp->limit.safetysw; input_disp = set_input_source_cb;
            break;
        case ITEM_SAFEVAL:
            label = _tr("Safe Val");
            disp = set_safeval_cb;
            break;
        case ITEM_MINLIMIT:
            label = _tr("Min Limit");
            disp = set_limits_cb; value = &mp->limit.min;
            break;
        case ITEM_MAXLIMIT:
            label = _tr("Max Limit");        
            disp = set_limits_cb; value = &mp->limit.max;
            break;
        case ITEM_SCALEPOS:
            label_cb = scalestring_cb; label = (void *)1L;
            disp = set_limitsscale_cb; value = &mp->limit.servoscale;
            break;
        case ITEM_SCALENEG:
            label_cb = scalestring_cb; label = (void *)0L;
            disp = set_limitsscale_cb; value = &mp->limit.servoscale_neg;
            break;
        case ITEM_SUBTRIM:
            label = _tr("Subtrim");
            disp = set_trimstep_cb; value = &mp->limit.subtrim;
            break;
        case ITEM_SPEED:
            label = _tr("Speed");
            disp = set_limits_cb; value = &mp->limit.speed;
            break;
    }
    labelDesc.style = LABEL_LEFT;
    GUI_CreateLabelBox(&gui->label[relrow], LABEL_X, y, LABEL_W, LINE_HEIGHT, &labelDesc, label_cb, NULL, label);
    labelDesc.style = LABEL_CENTER;
    GUI_CreateTextSourcePlate(&gui->value[relrow], TEXTSEL_X, y, TEXTSEL_W, LINE_HEIGHT, &labelDesc, tgl, disp, input_disp, value);

    if(absrow == ITEM_SAFEVAL)
        GUI_TextSelectEnable(&gui->value[relrow], mp->limit.safetysw);

    return 1;
}

static void _show_limits()
{
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, ITEM_LAST, row_cb, getobj_cb, NULL, NULL);
    GUI_SetSelected(GUI_GetScrollableObj(&gui->scrollable, ITEM_REVERSE, 0));
};

void revert_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    memcpy(&mp->limit, (const void *)&origin_limit, sizeof(origin_limit));
    MIXER_SetLimit(mp->channel, &mp->limit);  // save
    GUI_DrawScreen();
}

static unsigned action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_RemoveAllObjects();  // Discard unsaved items and exit to upper page
            PAGE_MixerInit(mp->top_channel);
        } else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

static inline guiObject_t *_get_obj(int idx, int objid) {
    return (guiObject_t *)GUI_GetScrollableObj(&gui->scrollable, idx, objid);
}
