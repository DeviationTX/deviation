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

#include "common.h"
#include "pages.h"
#include "telemetry.h"
#include "gui/gui.h"

#include "../common/_telemtest_page.c"

typedef enum {
    telemetry_off,
    telemetry_basic,
    telemetry_gps,
} TeleMetryMonitorType;

static u8 _action_cb(u32 button, u8 flags, void *data);
static const char *_page_cb(guiObject_t *obj, const void *data);
static void _press_cb(guiObject_t *obj, const void *data);
static void _show_page2();
static const char *idx_cb(guiObject_t *obj, const void *data);

static TeleMetryMonitorType current_page = telemetry_basic;

static void _show_page1()
{
    PAGE_RemoveAllObjects();
    u8 w = 35;
    PAGE_ShowHeader(_tr_noop("")); // to draw a underline only
    GUI_CreateLabelBox(&gui1->tempstr, 8, 0, w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, (void *)_tr("Temp:"));
    GUI_CreateLabelBox(&gui1->voltstr, w + 13, 0, w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, (void *)_tr("Volt:"));
    GUI_CreateLabelBox(&gui1->rpmstr, w + w + 18, 0, w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, (void *)_tr("RPM:"));
    w = 10;
    GUI_CreateLabelBox(&gui1->page, LCD_WIDTH -w, 0, w, 7, &TINY_FONT, _page_cb, NULL, NULL);

    u8 space = ITEM_HEIGHT +1;
    u8 row = space;
    w = 35;
    labelDesc.font = TINY_FONT.font;
    labelDesc.font_color = 0xffff;
    labelDesc.fill_color = 0;
    for(long i = 0; i < 4; i++) {
        u8 x = 8;
        labelDesc.style = LABEL_LEFTCENTER;
        GUI_CreateLabelBox(&gui1->idx[i], 0,  row, 8, ITEM_HEIGHT, &TINY_FONT, idx_cb, NULL, (void *)(long)i);
        labelDesc.style = LABEL_SQUAREBOX;
        GUI_CreateLabelBox(&gui1->temp[i], x,  row, w, ITEM_HEIGHT, &labelDesc,
                          telem_cb, NULL, (void *)(TELEM_TEMP1+i));
        if (i < 3) {
            x = x + w + 5;
            GUI_CreateLabelBox(&gui1->volt[i], x,  row, w, ITEM_HEIGHT, &labelDesc, telem_cb, NULL, (void *)(TELEM_VOLT1+i));
        }
        if (i < 2) {
            x = x + w + 5;
            GUI_CreateLabelBox(&gui1->rpm[i], x,  row, w, ITEM_HEIGHT, &labelDesc, telem_cb, NULL, (void *)(TELEM_RPM1+i));
        }
        row += space;
    }
    tp.telem = Telemetry;
    tp.telem.time[0] = 0;
    tp.telem.time[1] = 0;
    tp.telem.time[2] = 0;

    labelDesc.font = DEFAULT_FONT.font; // bug fix: quickpage(telem)->main page->main menu,all pages' font will be set to TINY_FONT
    labelDesc.font_color = 0xffff;
    labelDesc.outline_color = labelDesc.fill_color = 0; // bug fix: reset to default no-box style
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    (void)relrow;
    return (guiObject_t *)&gui2->gps[col];
}
static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    (void)relrow;
    labelDesc.font = TINY_FONT.font;
    labelDesc.style = LABEL_SQUAREBOX;
    labelDesc.font_color = 0xffff;
    labelDesc.fill_color = 0;
    absrow = absrow*2;
    for(int i = 0; i < 2 && absrow < 5; i++) {
        GUI_CreateLabelBox(&gui2->gpsstr[i], 0, y,
                0, ITEM_HEIGHT, &DEFAULT_FONT,  label_cb, NULL, (void *)(long)(TELEM_GPS_LAT+absrow));
        GUI_CreateLabelBox(&gui2->gps[i], 0, y + ITEM_HEIGHT + 1,
                LCD_WIDTH - ARROW_WIDTH - 3, ITEM_HEIGHT, &labelDesc, telem_cb, NULL, (void *)(long)(TELEM_GPS_LAT+absrow));
        y +=  2 *(ITEM_HEIGHT + 1);
        absrow +=1;
    }

    labelDesc.font = DEFAULT_FONT.font; // bug fix: quickpage(telem)->main page->main menu,all pages' font will be set to TINY_FONT
    labelDesc.font_color = 0xffff;
    labelDesc.outline_color = labelDesc.fill_color = 0; // bug fix: reset to default no-box style
    return 0;
}
static void _show_page2()
{
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr_noop("GPS")); // to draw a underline only
    u8 w = 10;
    GUI_CreateLabelBox(&gui2->page, LCD_WIDTH -w, 0, w, 7, &TINY_FONT, _page_cb, NULL, NULL);

    tp.telem = Telemetry;
    tp.telem.time[0] = 0;
    tp.telem.time[1] = 0;
    tp.telem.time[2] = 0;

    GUI_CreateScrollable(&gui2->scrollable, 0, ITEM_HEIGHT + 1, LCD_WIDTH, LCD_HEIGHT - ITEM_HEIGHT -1,
                         4 * ITEM_HEIGHT + 4, 3, row_cb, getobj_cb, NULL, NULL);
}

static const char *idx_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    sprintf(tp.str, "%d", idx+1);
    return tp.str;
}

void PAGE_TelemtestInit(int page)
{
    (void)okcancel_cb;
    (void)page;
    PAGE_SetModal(0);
    PAGE_SetActionCB(_action_cb);
    if (telem_state_check() == 0) {
        current_page = telemetry_off;
        GUI_CreateLabelBox(&gui1->msg, 20, 10, 0, 0, &DEFAULT_FONT, NULL, NULL, tp.str);
        return;
    }

    if (current_page== telemetry_gps)
        _show_page2();
    else
        _show_page1();
}

void PAGE_TelemtestModal(void(*return_page)(int page), int page)
{
    (void)return_page;
    (void)page;
}

static const char *_page_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if (current_page== telemetry_gps)
        strcpy(tp.str, (const char *)"<-");
    else
        strcpy(tp.str, (const char *)"->");
    return tp.str;
}

static void _press_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    current_page = current_page == telemetry_gps?telemetry_basic: telemetry_gps;
    if (current_page== telemetry_gps)
        _show_page2();
    else
        _show_page1();
}

static void _navigate_pages(s8 direction)
{
    if ((direction == -1 && current_page == telemetry_gps) ||
            (direction == 1 && current_page == telemetry_basic)) {
        _press_cb(NULL, NULL);
    }
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            labelDesc.font = DEFAULT_FONT.font;  // set it back to 12x12 font
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        } else if (current_page != telemetry_off) {
            // this indicates whether telem is off or not supported
            if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
                _navigate_pages(1);
            }  else if (CHAN_ButtonIsPressed(button,BUT_LEFT)) {
                _navigate_pages(-1);
            } else {
                return 0;
            }
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
static inline guiObject_t *_get_obj(int idx, int objid) {
    return GUI_GetScrollableObj(&gui2->scrollable, idx, objid);
}
