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
    telemetry_basic,
    telemetry_gps,
} TeleMetryMonitorType;

static u8 _action_cb(u32 button, u8 flags, void *data);
static const char *_page_cb(guiObject_t *obj, const void *data);
static void _press_cb(guiObject_t *obj, const void *data);
static void _show_page2();
static const char *idx_cb(guiObject_t *obj, const void *data);

#define VIEW_ID 0
static TeleMetryMonitorType current_page = telemetry_basic;
static s8 current_item = 0;

static void _show_page1()
{
    PAGE_RemoveAllObjects();
    memset(gui, 0, sizeof(*gui));
    u8 w = 35;
    PAGE_ShowHeader(_tr_noop("")); // to draw a underline only
    GUI_CreateLabelBox(&gui->tempstr, 8, 0, w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, (void *)_tr("Temp:"));
    GUI_CreateLabelBox(&gui->voltstr, w + 13, 0, w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, (void *)_tr("Volt:"));
    GUI_CreateLabelBox(&gui->rpmstr, w + w + 18, 0, w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, (void *)_tr("RPM:"));
    w = 10;
    GUI_CreateLabelBox(&gui->page, LCD_WIDTH -w, 0, w, 7, &TINY_FONT, _page_cb, NULL, NULL);

    u8 space = ITEM_HEIGHT +1;
    u8 row = space;
    w = 35;
    labelDesc.font = TINY_FONT.font;
    labelDesc.font_color = 0xffff;
    labelDesc.fill_color = 0;
    for(long i = 0; i < 4; i++) {
        u8 x = 8;
        labelDesc.style = LABEL_LEFTCENTER;
        GUI_CreateLabelBox(&gui->idx[i], 0,  row, 8, ITEM_HEIGHT, &TINY_FONT, idx_cb, NULL, (void *)(long)i);
        labelDesc.style = LABEL_SQUAREBOX;
        GUI_CreateLabelBox(&gui->temp[i], x,  row, w, ITEM_HEIGHT, &labelDesc,
                          telem_cb, NULL, (void *)(TELEM_TEMP1+i));
        if (i < 3) {
            x = x + w + 5;
            GUI_CreateLabelBox(&gui->volt[i], x,  row, w, ITEM_HEIGHT, &labelDesc, telem_cb, NULL, (void *)(TELEM_VOLT1+i));
        }
        if (i < 2) {
            x = x + w + 5;
            GUI_CreateLabelBox(&gui->rpm[i], x,  row, w, ITEM_HEIGHT, &labelDesc, telem_cb, NULL, (void *)(TELEM_RPM1+i));
        }
        row += space;
    }
    tp.telem = Telemetry;
    tp.telem.time[0] = 0;
    tp.telem.time[1] = 0;
    tp.telem.time[2] = 0;
    // bug fix: scroll_bar must be initialized, otherwise it will caused crash when checked against NULL(press UP/DOWN keys)
    OBJ_SET_USED(&gui->scroll, 0);
    labelDesc.font = DEFAULT_FONT.font; // bug fix: quickpage(telem)->main page->main menu,all pages' font will be set to TINY_FONT
    labelDesc.font_color = 0xffff;
    labelDesc.outline_color = labelDesc.fill_color = 0; // bug fix: reset to default no-box style
}

static void _show_page2()
{
    PAGE_RemoveAllObjects();
    memset(gui, 0, sizeof(*gui));
    current_item = 0;
    PAGE_ShowHeader(_tr_noop("GPS")); // to draw a underline only
    u8 w = 10;
    GUI_CreateLabelBox(&gui->page, LCD_WIDTH -w, 0, w, 7, &TINY_FONT, _page_cb, NULL, NULL);

    // Create a logical view
    u8 space = ITEM_HEIGHT + 1;
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = space;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH -ARROW_WIDTH, LCD_HEIGHT - space ,
            view_origin_absoluteX, view_origin_absoluteY);

    u8 row = 0;
    labelDesc.font = TINY_FONT.font;
    labelDesc.style = LABEL_SQUAREBOX;
    labelDesc.font_color = 0xffff;
    labelDesc.fill_color = 0;
    for(long i = 0; i < 5; i++) {
        GUI_CreateLabelBox(&gui->gpsstr[i], GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &DEFAULT_FONT,  label_cb, NULL, (void *)(TELEM_GPS_LAT+i));
        row += space;
        GUI_CreateLabelBox(&gui->gps[i], GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                LCD_WIDTH - ARROW_WIDTH - 3, ITEM_HEIGHT, &labelDesc, telem_cb, NULL, (void *)(TELEM_GPS_LAT+i));
        row += space;

    }
    tp.telem = Telemetry;
    tp.telem.time[0] = 0;
    tp.telem.time[1] = 0;
    tp.telem.time[2] = 0;
    GUI_CreateScrollbar(&gui->scroll, LCD_WIDTH - ARROW_WIDTH, ITEM_HEIGHT, LCD_HEIGHT- ITEM_HEIGHT, 3, NULL, NULL, NULL);
    labelDesc.font = DEFAULT_FONT.font; // bug fix: quickpage(telem)->main page->main menu,all pages' font will be set to TINY_FONT
    labelDesc.font_color = 0xffff;
    labelDesc.outline_color = labelDesc.fill_color = 0; // bug fix: reset to default no-box style
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
        GUI_CreateLabelBox(&gui->msg, 20, 10, 0, 0, &DEFAULT_FONT, NULL, NULL, tp.str);
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

static void _navigate_items(s8 direction)
{
    if (OBJ_IS_USED(&gui->scroll)) // no page scroll
        return;
    current_item += direction;
    if (current_item <=0) {
        current_item = 0;
        GUI_SetRelativeOrigin(VIEW_ID, 0, 0);
    }  else if (current_item > 2) {
        current_item = 2;
    } else {
        GUI_ScrollLogicalView(VIEW_ID, direction *(LCD_HEIGHT - ITEM_HEIGHT));
    }
    GUI_SetScrollbar(&gui->scroll, current_item);
    //GUI_Redraw(scroll_bar); // must redraw the scroll_bar as the page event keeps refreshing this page
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
        } else if (OBJ_IS_USED(&gui->volt[0]) || OBJ_IS_USED(&gui->gps[0])){ // this indicates whether telem is off or not supported
            if (CHAN_ButtonIsPressed(button, BUT_UP)) {
                _navigate_items(-1);
            }  else if (CHAN_ButtonIsPressed(button,BUT_DOWN)) {
                _navigate_items(1);
            } else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
                _navigate_pages(1);
            }  else if (CHAN_ButtonIsPressed(button,BUT_LEFT)) {
                _navigate_pages(-1);
            }
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
