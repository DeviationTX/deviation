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
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "telemetry.h"

enum {
    NEWTEXTSEL_X = 0,
    NEWTEXTSEL_W = 68,
    NEWBUTTON_X = 69,
    NEWBUTTON_W = 54,
    QPBUTTON_X  = 0,
    QPBUTTON_W  = 123,
    MENULABEL_X = 0,
    MENULABEL_W = 56,
    MENUTEXT_X  = 0,
    MENUTEXT_W  = 123,
    #define LINE_OFFSET 1
    ELEMBUT_X   = 0,
    ELEMBUT_W   = 55,
    ELEMLBL_X   = 0,
    ELEMLBL_W   = 56,
    ELEMTXT_X   = 56,
    ELEMTXT_W   = 67,
};
#endif //OVERRIDE_PLACEMENT
static struct layout_page    * const lp  = &pagemem.u.layout_page;
static struct PageCfg2       * const pc  = &Model.pagecfg2;
static struct mainconfig_obj * const gui = &gui_objs.u.mainconfig;
static u16 current_selected = 0;

static const int HEADER_Y = 10;

#include "../common/_main_config.c"

#if HAS_LAYOUT_EDITOR
static unsigned _action_cb(u32 button, unsigned flags, void *data);
#endif

void PAGE_MainLayoutInit(int page)
{
    (void)page;
#if HAS_LAYOUT_EDITOR
    PAGE_ShowHeader(_tr_noop("Layout: Long-Press ENT"));
    PAGE_SetActionCB(_action_cb);
#else
    PAGE_ShowHeader(PAGE_GetName(PAGEID_MAINCFG));
#endif
    memset(gui, 0, sizeof(*gui));
    memset(lp, 0, sizeof(*lp));
    show_config();
}

static int size_cb(int absrow, void *data)
{
    int num_elems = (long)data;
    return (absrow >= num_elems  && absrow < num_elems + NUM_QUICKPAGES) ? 1 + LINE_OFFSET : 1;
}

static const char *cfglabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    int type = ELEM_TYPE(pc->elem[i]);
    int idx = elem_abs_to_rel(i);
    const char *str;
    switch(type) {
    case ELEM_VTRIM:
    case ELEM_HTRIM:
        str = _tr("Trimbar");
        break; 
    case ELEM_BIGBOX:
    case ELEM_SMALLBOX:
        str = _tr("Box");
        break;
    case ELEM_BAR:
        str = _tr("Bar");
        break;
    case ELEM_TOGGLE:
        str = _tr("Toggle");
        break;
    default:
        str = _tr(GetElemName(type));
        break;
    }
    sprintf(tempstring,"%s%d", str, idx+1);
    return tempstring;
}

static void switchicon_press_cb(guiObject_t *obj, const void *data)
{
    PAGE_SaveCurrentPos();
    TGLICO_Select(obj, data);
}

void newelem_press_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    //PAGE_MainLayoutExit();
    create_element();
    current_selected = 0;
    PAGE_ChangeByID(PAGEID_MAINCFG, 0);
}

static const char *dlgts1_cb(guiObject_t *obj, int dir, void *data)
{
    int idx = (long)data;
    if (pc->elem[idx].src == 0 && dir < 0)
        pc->elem[idx].src = -1;
    if ((s8)pc->elem[idx].src == -1 && dir > 0) {
        pc->elem[idx].src = 0;
        dir = 0;
    }
    if ((s8)pc->elem[idx].src < 0) {
        GUI_TextSelectEnablePress((guiTextSelect_t *)obj, 1);
        //PAGE_MainLayoutExit();
        return _tr("Delete");
    }
    GUI_TextSelectEnablePress((guiTextSelect_t *)obj, 0);
    return dlgts_cb(obj, dir, data);
}

static void add_dlgbut_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    if(!data) {
        PAGE_PushByID(PAGEID_LOADSAVE, LOAD_LAYOUT);
    }
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    int num_elems = (long)data;
    int y_ts = y;
    //show elements in order
    int row = -1;
#if HAS_LAYOUT_EDITOR
    if (absrow == num_elems + NUM_QUICKPAGES) {
        GUI_CreateTextSelectPlate(&gui->value[relrow], NEWTEXTSEL_X, y_ts,
                 NEWTEXTSEL_W, LINE_HEIGHT, &TEXTSEL_FONT, NULL, newelem_cb, NULL);
        GUI_CreateButtonPlateText(&gui->col1[relrow].button, NEWBUTTON_X, y,  NEWBUTTON_W,
                 LINE_HEIGHT, &BUTTON_FONT, add_dlgbut_str_cb, newelem_press_cb, (void *)1);
        return 2;
    }
#endif
    if (absrow >= num_elems + NUM_QUICKPAGES) {
        GUI_CreateButtonPlateText(&gui->col1[relrow].button, QPBUTTON_X, y, QPBUTTON_W, LINE_HEIGHT,
                 &BUTTON_FONT, add_dlgbut_str_cb, add_dlgbut_cb, (void *)0);
        return 1;
    }
    if (absrow >= num_elems && absrow < num_elems + NUM_QUICKPAGES) {
        GUI_CreateLabelBox(&gui->col1[relrow].label, MENULABEL_X, y,  MENULABEL_W, LINE_HEIGHT, &LABEL_FONT, menulabel_cb, NULL, (void *)(long)(absrow - num_elems));
        GUI_CreateTextSelectPlate(&gui->value[relrow], MENUTEXT_X, y + LINE_HEIGHT * LINE_OFFSET,
             MENUTEXT_W, LINE_HEIGHT, &TEXTSEL_FONT, NULL, menusel_cb, (void *)(long)(absrow - num_elems));
        return 1;
    }
    for(int type = 0; type < ELEM_LAST; type++) {
        if (type == ELEM_BIGBOX || type == ELEM_HTRIM)
            continue;
        long nxt = -1;
        long item = -1;
        while((nxt = MAINPAGE_FindNextElem(type, nxt+1)) >= 0) {
            item = nxt;
            row++;
            if(row == absrow)
                break;
        }
        if (nxt == -1)
            continue;

        int selectable = 1;
        if (type == ELEM_TOGGLE) {
            GUI_CreateButtonPlateText(&gui->col1[relrow].button, ELEMBUT_X, y, ELEMBUT_W,
                    LINE_HEIGHT, &BUTTON_FONT, cfglabel_cb, switchicon_press_cb, (void *)item);
            selectable = 2;
        }
        else
            GUI_CreateLabelBox(&gui->col1[relrow].label, ELEMLBL_X, y, ELEMLBL_W, LINE_HEIGHT, &LABEL_FONT, cfglabel_cb, NULL, (void *)item);

        GUI_CreateTextSelectPlate(&gui->value[relrow], ELEMTXT_X, y_ts,
             ELEMTXT_W, LINE_HEIGHT, &TEXTSEL_FONT, (void(*)(guiObject_t *, void *))dlgbut_cb, dlgts1_cb, (void *)item);
        return selectable;
    }
    return 0;
}

void show_config()
{
    long count = 0;
    for (count = 0; count < NUM_ELEMS; count++) {
        if (! ELEM_USED(pc->elem[count]))
            break;
    }
#if HAS_LAYOUT_EDITOR
    static const int ADD_LOAD = 2;
# else
    static const int ADD_LOAD = 1;
#endif
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LINE_SPACE, count + NUM_QUICKPAGES + ADD_LOAD, row_cb, NULL, size_cb, (void *)count);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}

#if HAS_LAYOUT_EDITOR
static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if (CHAN_ButtonIsPressed(button, BUT_ENTER) &&(flags & BUTTON_LONGPRESS)) {
        PAGE_PushByID(PAGEID_LAYOUT, 0);
        return 1;
    }
    return default_button_action_cb(button, flags, data);
}
#endif //HAS_LAYOUT_EDITOR
