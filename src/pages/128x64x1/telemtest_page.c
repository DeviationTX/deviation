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
#include "telemetry.h"
#include "gui/gui.h"

enum {
    ITEM1_X      = 8,
    ITEM1_WIDTH  = 35,
    ITEM2_X      = 48,
    ITEM3_X      = 87,
    ARROW_X      = LCD_WIDTH - 11,
    ARROW_W      = 10,
    //
    LBL1_X       = 0,
    LBL1_WIDTH   = 8,
    LBL2_X       = 43,
    LBL3_X       = 86,
    LBL4_X       = 0,
    LBL4_WIDTH   = 25,
    LBL5_X       = 61,
    //
    HEADER_X     = 90,
    HEADER_WIDTH = 35,
    //
    GPS_X        = 0,
    GPS_WIDTH    = LCD_WIDTH - ARROW_WIDTH - 3,
    //
    DSM1_X       = 7,
    DSM1_WIDTH   = 35,
    DSM2_X       = 50,
    DSM3_X       = 93,
    DSM4_X       = 25,
    DSM5_X       = 86,
    //
    FRSKY1_X     = 8,
    FRSKY1_WIDTH = 35,
    FRSKY2_X     = 48,
    FRSKY3_X     = 86,
};
#endif //OVERRIDE_PLACEMENT
#if HAS_TELEMETRY
#include "../common/_telemtest_page.c"

typedef enum {
    telemetry_basic,
    telemetry_gps,
    telemetry_off,
} TeleMetryMonitorType;

static unsigned _action_cb(u32 button, unsigned flags, void *data);
static void _press_cb(guiObject_t *obj, const void *data);
static const char *idx_cb(guiObject_t *obj, const void *data);

static TeleMetryMonitorType current_page = telemetry_basic;

struct telem_layout {
    u8 row_type;
    u8 x;
    u8 width;
    u8 source;
};
enum {
    TYPE_INDEX  = 0x10,
    TYPE_HEADER = 0x20,
    TYPE_LABEL  = 0x30,
    TYPE_LABEL3 = 0x40,
    TYPE_VALUE  = 0x80,
    TYPE_VALUE2 = 0x90,
    TYPE_VALUE4 = 0xa0,
};

enum {
    TEMP_LABEL = 1,
    VOLT_LABEL,
    RPM_LABEL,
    GPS_LABEL,
    A_LABEL,
    B_LABEL,
    L_LABEL,
    R_LABEL,
    F_LABEL,
    H_LABEL,
    RXV_LABEL,
    BATT_LABEL,
    DSM_LABEL,
    CELLS_LABEL,
    MISC_LABEL,
    ARROW_LABEL = 0xff,
};

struct telem_layout2 {
    const struct telem_layout *header;
    const struct telem_layout *layout;
    u8 num_items;
    u8 row_height;
};

const struct telem_layout devo_header_basic[] = {
        {TYPE_HEADER, ITEM1_X, ITEM1_WIDTH, TEMP_LABEL},
        {TYPE_HEADER, ITEM2_X, ITEM1_WIDTH, VOLT_LABEL},
        {TYPE_HEADER, ITEM3_X, ITEM1_WIDTH, RPM_LABEL},
        {TYPE_HEADER, ARROW_X, ARROW_W,     ARROW_LABEL},
        {0, 0, 0, 0},
};

const struct telem_layout devo_layout_basic[] = {
    {TYPE_INDEX | 0, LBL1_X, LBL1_WIDTH,  1},
    {TYPE_VALUE | 0, ITEM1_X, ITEM1_WIDTH, TELEM_DEVO_TEMP1},
    {TYPE_VALUE | 0, ITEM2_X, ITEM1_WIDTH, TELEM_DEVO_VOLT1},
    {TYPE_VALUE | 0, ITEM3_X, ITEM1_WIDTH, TELEM_DEVO_RPM1},
    {TYPE_INDEX | 1, LBL1_X, LBL1_WIDTH,  2},
    {TYPE_VALUE | 1, ITEM1_X, ITEM1_WIDTH, TELEM_DEVO_TEMP2},
    {TYPE_VALUE | 1, ITEM2_X, ITEM1_WIDTH, TELEM_DEVO_VOLT2},
    {TYPE_VALUE | 1, ITEM3_X, ITEM1_WIDTH, TELEM_DEVO_RPM2},
    {TYPE_INDEX | 2, LBL1_X, LBL1_WIDTH,  3},
    {TYPE_VALUE | 2, ITEM1_X, ITEM1_WIDTH, TELEM_DEVO_TEMP3},
    {TYPE_VALUE | 2, ITEM2_X, ITEM1_WIDTH, TELEM_DEVO_VOLT3},
    {TYPE_INDEX | 3, LBL1_X, LBL1_WIDTH,  4},
    {TYPE_VALUE | 3, ITEM1_X, ITEM1_WIDTH, TELEM_DEVO_TEMP4},
    {0, 0, 0, 0},
};

const struct telem_layout devo_header_gps[] = {
        {TYPE_HEADER, HEADER_X, HEADER_WIDTH, GPS_LABEL},
        {TYPE_HEADER, ARROW_X,  ARROW_W,     ARROW_LABEL},
        {0, 0, 0, 0},
};
const struct telem_layout devo_layout_gps[] = {
    {TYPE_LABEL  | 0, GPS_X, 0,  TELEM_GPS_LAT},
    {TYPE_VALUE2 | 0, GPS_X, GPS_WIDTH, TELEM_GPS_LAT},
    {TYPE_LABEL3 | 0, GPS_X, 0,  TELEM_GPS_LONG},
    {TYPE_VALUE4 | 0, GPS_X, GPS_WIDTH, TELEM_GPS_LONG},

    {TYPE_LABEL  | 1, GPS_X, 0,  TELEM_GPS_ALT},
    {TYPE_VALUE2 | 1, GPS_X, GPS_WIDTH, TELEM_GPS_ALT},
    {TYPE_LABEL3 | 1, GPS_X, 0,  TELEM_GPS_SPEED},
    {TYPE_VALUE4 | 1, GPS_X, GPS_WIDTH, TELEM_GPS_SPEED},

    {TYPE_LABEL  | 2, GPS_X, 0,  TELEM_GPS_TIME},
    {TYPE_VALUE2 | 2, GPS_X, GPS_WIDTH, TELEM_GPS_TIME},
    {0, 0, 0, 0},
};

const struct telem_layout dsm_header_basic[] = {
        {TYPE_HEADER, HEADER_X, HEADER_WIDTH, DSM_LABEL},
        {TYPE_HEADER, ARROW_X, ARROW_W,     ARROW_LABEL},
        {0, 0, 0, 0},
};

const struct telem_layout dsm_layout_basic[] = {
    {TYPE_HEADER | 0, LBL1_X, LBL1_WIDTH,  A_LABEL},
    {TYPE_VALUE  | 0, DSM1_X, DSM1_WIDTH, TELEM_DSM_FLOG_FADESA},
    {TYPE_HEADER | 0, LBL2_X, LBL1_WIDTH,  B_LABEL},
    {TYPE_VALUE  | 0, DSM2_X, DSM1_WIDTH, TELEM_DSM_FLOG_FADESB},
    {TYPE_HEADER | 0, LBL3_X, LBL1_WIDTH,  F_LABEL},
    {TYPE_VALUE  | 0, DSM3_X, DSM1_WIDTH, TELEM_DSM_FLOG_FRAMELOSS},

    {TYPE_HEADER | 1, LBL1_X, LBL1_WIDTH,  L_LABEL},
    {TYPE_VALUE  | 1, DSM1_X, DSM1_WIDTH, TELEM_DSM_FLOG_FADESL},
    {TYPE_HEADER | 1, LBL2_X, LBL1_WIDTH,  R_LABEL},
    {TYPE_VALUE  | 1, DSM2_X, DSM1_WIDTH, TELEM_DSM_FLOG_FADESR},
    {TYPE_HEADER | 1, LBL3_X, LBL1_WIDTH,  H_LABEL},
    {TYPE_VALUE  | 1, DSM3_X, DSM1_WIDTH, TELEM_DSM_FLOG_HOLDS},

    {TYPE_HEADER | 2, LBL4_X, LBL4_WIDTH, TEMP_LABEL},
    {TYPE_VALUE  | 2, DSM4_X, DSM1_WIDTH, TELEM_DSM_FLOG_TEMP1},
    {TYPE_HEADER | 2, LBL5_X, LBL4_WIDTH, RXV_LABEL},
    {TYPE_VALUE  | 2, DSM5_X, DSM1_WIDTH, TELEM_DSM_FLOG_VOLT1},

    {TYPE_HEADER | 3, LBL4_X, LBL4_WIDTH, BATT_LABEL},
    {TYPE_VALUE  | 3, DSM4_X, DSM1_WIDTH, TELEM_DSM_FLOG_VOLT2},
    {TYPE_HEADER | 3, LBL5_X, LBL4_WIDTH, RPM_LABEL},
    {TYPE_VALUE  | 3, DSM5_X, DSM1_WIDTH, TELEM_DSM_FLOG_RPM1},

    {0, 0, 0, 0},
};

const struct telem_layout frsky_header_basic[] = {
        {TYPE_HEADER, ITEM1_X, ITEM1_WIDTH, MISC_LABEL},
        {TYPE_HEADER, ITEM2_X, ITEM1_WIDTH, BATT_LABEL},
        {TYPE_HEADER, ITEM3_X, ITEM1_WIDTH, CELLS_LABEL},
        {TYPE_HEADER, ARROW_X, ARROW_W,     ARROW_LABEL},
        {0, 0, 0, 0},
};

const struct telem_layout frsky_layout_basic[] = {
    {TYPE_INDEX | 0, LBL1_X, LBL1_WIDTH,  1},
    {TYPE_VALUE | 0, FRSKY1_X, FRSKY1_WIDTH, TELEM_FRSKY_RSSI},
    {TYPE_VALUE | 0, FRSKY2_X, FRSKY1_WIDTH, TELEM_FRSKY_VOLT1},
#if HAS_EXTENDED_TELEMETRY
    {TYPE_VALUE | 0, FRSKY3_X, FRSKY1_WIDTH, TELEM_FRSKY_CELL1},
#else
    {TYPE_VALUE | 0, FRSKY3_X, FRSKY1_WIDTH, TELEM_FRSKY_LQI},
#endif

    {TYPE_INDEX | 1, LBL1_X, LBL1_WIDTH,  2},
#if HAS_EXTENDED_TELEMETRY
    {TYPE_VALUE | 1, FRSKY1_X, FRSKY1_WIDTH, TELEM_FRSKY_TEMP1},
#endif
    {TYPE_VALUE | 1, FRSKY2_X, FRSKY1_WIDTH, TELEM_FRSKY_VOLT2},
#if HAS_EXTENDED_TELEMETRY
    {TYPE_VALUE | 1, FRSKY3_X, FRSKY1_WIDTH, TELEM_FRSKY_CELL2},
#else
    {TYPE_VALUE | 1, FRSKY3_X, FRSKY1_WIDTH, TELEM_FRSKY_LRSSI},
#endif

#if HAS_EXTENDED_TELEMETRY
    {TYPE_INDEX | 2, LBL1_X, LBL1_WIDTH,  3},
    {TYPE_VALUE | 2, FRSKY1_X, FRSKY1_WIDTH, TELEM_FRSKY_TEMP2},
    {TYPE_VALUE | 2, FRSKY2_X, FRSKY1_WIDTH, TELEM_FRSKY_VOLT3},
    {TYPE_VALUE | 2, FRSKY3_X, FRSKY1_WIDTH, TELEM_FRSKY_CELL3},

    {TYPE_INDEX | 3, LBL1_X, LBL1_WIDTH, 4},
    {TYPE_VALUE | 3, FRSKY1_X, FRSKY1_WIDTH, TELEM_FRSKY_RPM},
    {TYPE_VALUE | 3, FRSKY2_X, FRSKY1_WIDTH, TELEM_FRSKY_MIN_CELL},
    {TYPE_VALUE | 3, FRSKY3_X, FRSKY1_WIDTH, TELEM_FRSKY_CELL4},

    {TYPE_INDEX | 4, LBL1_X, LBL1_WIDTH, 5},
    {TYPE_VALUE | 4, FRSKY1_X, FRSKY1_WIDTH, TELEM_FRSKY_FUEL},
    {TYPE_VALUE | 4, FRSKY2_X, FRSKY1_WIDTH, TELEM_FRSKY_ALL_CELL},
    {TYPE_VALUE | 4, FRSKY3_X, FRSKY1_WIDTH, TELEM_FRSKY_CELL5},

    {TYPE_INDEX | 5, LBL1_X, LBL1_WIDTH, 6},
    {TYPE_VALUE | 5, FRSKY1_X, FRSKY1_WIDTH, TELEM_FRSKY_ALTITUDE},
    {TYPE_VALUE | 5, FRSKY2_X, FRSKY1_WIDTH, TELEM_FRSKY_VOLTA},
    {TYPE_VALUE | 5, FRSKY3_X, FRSKY1_WIDTH, TELEM_FRSKY_CELL6},

    {TYPE_INDEX | 6, LBL1_X, LBL1_WIDTH, 7},
    {TYPE_VALUE | 6, FRSKY1_X, FRSKY1_WIDTH, TELEM_FRSKY_VARIO},
    {TYPE_VALUE | 6, FRSKY2_X, FRSKY1_WIDTH, TELEM_FRSKY_CURRENT},
    {TYPE_VALUE | 6, FRSKY3_X, FRSKY1_WIDTH, TELEM_FRSKY_DISCHARGE},

    {TYPE_INDEX | 7, LBL1_X, LBL1_WIDTH, 8},
    {TYPE_VALUE | 7, FRSKY1_X, FRSKY1_WIDTH, TELEM_FRSKY_LQI},
    {TYPE_VALUE | 7, FRSKY2_X, FRSKY1_WIDTH, TELEM_FRSKY_LRSSI},
#endif

    {0, 0, 0, 0},
};

const struct telem_layout2 devo_page[] = {
    {devo_header_basic, devo_layout_basic, 4, 1},
    {devo_header_gps, devo_layout_gps, 3, 4},
};
const struct telem_layout2 dsm_page[] = {
    {dsm_header_basic, dsm_layout_basic, 4, 1},
    {devo_header_gps, devo_layout_gps, 3, 4},
};
const struct telem_layout2 frsky_page[] = {
#if HAS_EXTENDED_TELEMETRY
    {frsky_header_basic, frsky_layout_basic, 8, 1},
#else
    {frsky_header_basic, frsky_layout_basic, 2, 1},
#endif
    {devo_header_gps, devo_layout_gps, 3, 4},
};
static const char *header_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int source = (long)data;
    switch(source) {
        case TEMP_LABEL: return _tr("Temp");
        case VOLT_LABEL: return _tr("Volt");
        case RPM_LABEL: return _tr("RPM");
        case GPS_LABEL: return _tr("GPS");
        case A_LABEL: return "A";
        case B_LABEL: return "B";
        case L_LABEL: return "L";
        case R_LABEL: return "R";
        case F_LABEL: return "F";
        case H_LABEL: return "H";
        case RXV_LABEL: return "RxV";
        case BATT_LABEL: return "Bat";
        case DSM_LABEL: return "DSM";
#if HAS_EXTENDED_TELEMETRY
        case CELLS_LABEL:return "Cells";
#else
        case CELLS_LABEL:return "Signl";
#endif
        case MISC_LABEL: return "Misc";
        case ARROW_LABEL: return current_page== telemetry_gps ? "<-" : "->";
    }
    return "";
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    (void)relrow;
    (void)col;
    return (guiObject_t *)&gui->box[0];
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)relrow;
    const struct telem_layout *layout = (const struct telem_layout *)data;
    int i = 0;
    struct LabelDesc *font;
    void *cmd = NULL;
    int orig_y = y;
    for(const struct telem_layout *ptr = layout; ptr->source; ptr++, i++) {
        if((ptr->row_type & 0x0f) < absrow)
            continue;
        if((ptr->row_type & 0x0f) > absrow)
            break;
        y = orig_y;
        font = &DEFAULT_FONT;
        switch (ptr->row_type & 0xf0) {
            case TYPE_INDEX:  font = &TINY_FONT; cmd = idx_cb; break;
            case TYPE_HEADER: cmd = header_cb; break;
            case TYPE_LABEL:  cmd = label_cb; break;
            case TYPE_LABEL3: cmd = label_cb; y = orig_y + 2*LINE_HEIGHT; break;
            case TYPE_VALUE:  font = &tp->font;  cmd = telem_cb; break;
            case TYPE_VALUE2: font = &tp->font;  cmd = telem_cb; y = orig_y + LINE_HEIGHT; break;
            case TYPE_VALUE4: font = &tp->font;  cmd = telem_cb; y = orig_y + 3*LINE_HEIGHT; break;
        }
        GUI_CreateLabelBox(&gui->box[i], ptr->x, y, ptr->width, LINE_HEIGHT,
                font, cmd, NULL, (void *)(long)ptr->source);
    }
    return 0;
}

static const struct telem_layout2 *_get_telem_layout2()
{
    const struct telem_layout2 *page;
    if (TELEMETRY_Type() == TELEM_DEVO)
        page = &devo_page[current_page];
    else if (TELEMETRY_Type() == TELEM_DSM)
        page = &dsm_page[current_page];
    else
        page = &frsky_page[current_page];
    return page;
}

static void _show_page()
{
    const struct telem_layout2 *page = _get_telem_layout2();
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader((page->header==devo_header_basic
                     || page->header==frsky_header_basic)
                    ? "" : _tr("Telemetry monitor"));
    tp->font.font = TINY_FONT.font;
    tp->font.font_color = 0xffff;
    tp->font.fill_color = 0;
    tp->font.style = LABEL_SQUAREBOX;
    DEFAULT_FONT.style = LABEL_CENTER;
    long i = 0;
    for(const struct telem_layout *ptr = page->header; ptr->source; ptr++, i++) {
        GUI_CreateLabelBox(&gui->header[i], ptr->x, 0, ptr->width, HEADER_HEIGHT,
                           ptr->source == ARROW_LABEL ? &NARROW_FONT : &DEFAULT_FONT,
                           header_cb, NULL, (void *)(long)ptr->source);
    }
    DEFAULT_FONT.style = LABEL_RIGHT;
    u8 row_height = page->row_height * LINE_SPACE;
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         row_height, page->num_items, row_cb, getobj_cb, NULL, (void *)page->layout);
    DEFAULT_FONT.style = LABEL_LEFT;
    tp->telem = Telemetry;
}

static const char *idx_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    sprintf(tempstring, "%d", idx);
    return tempstring;
}

void PAGE_ShowTelemetryAlarm()
{
    int cur_page = PAGE_GetID();
    if (cur_page != PAGEID_TELEMMON && cur_page != PAGEID_TELEMCFG)
        PAGE_ChangeByID(PAGEID_TELEMMON, PREVIOUS_ITEM);
}

void PAGE_TelemtestInit(int page)
{
    (void)okcancel_cb;
    (void)page;
    PAGE_SetModal(0);
    PAGE_SetActionCB(_action_cb);
    if (telem_state_check() == 0) {
        current_page = telemetry_off;
        GUI_CreateLabelBox(&gui->msg, 20, 10, 0, 0, &DEFAULT_FONT, NULL, NULL, tempstring);
        return;
    }
    if (current_page > telemetry_gps)
        current_page = telemetry_basic;

    _show_page();
}

void PAGE_TelemtestEvent() {
    if (current_page == telemetry_off)
        return;
    static u32 count;
    int flicker = ((++count & 3) == 0);
    struct Telemetry cur_telem = Telemetry;
    int current_row = GUI_ScrollableCurrentRow(&gui->scrollable);
    int visible_rows = GUI_ScrollableVisibleRows(&gui->scrollable);
    const struct telem_layout *ptr = _get_telem_layout2()->layout;
    for (long i = 0; ptr->source; ptr++, i++) {
        if ((ptr->row_type & 0x0f) < current_row)
            continue;
        if ((ptr->row_type & 0x0f) >= current_row + visible_rows)
            break;
        if (!( ptr->row_type & 0x80))
            continue;
        long cur_val = _TELEMETRY_GetValue(&cur_telem, ptr->source);
        long last_val = _TELEMETRY_GetValue(&tp->telem, ptr->source);
        struct LabelDesc *font;
        font = &TELEM_FONT;
        if((TELEMETRY_HasAlarm(ptr->source) && flicker) || ! TELEMETRY_IsUpdated(ptr->source)) {
            font = &TELEM_ERR_FONT;
        } else if (cur_val != last_val) {
            GUI_Redraw(&gui->box[i]);
        }
        GUI_SetLabelDesc(&gui->box[i], font);
    }
    tp->telem = cur_telem;
}

void PAGE_TelemtestModal(void(*return_page)(int page), int page)
{
    (void)return_page;
    (void)page;
}

static void _press_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    current_page = current_page == telemetry_gps?telemetry_basic: telemetry_gps;
    _show_page();
}

static void _navigate_pages(s8 direction)
{
    if ((direction == -1 && current_page == telemetry_gps) ||
            (direction == 1 && current_page == telemetry_basic)) {
        _press_cb(NULL, NULL);
    }
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if (flags & BUTTON_PRESS) {
        if (CHAN_ButtonIsPressed(button, BUT_ENTER) || CHAN_ButtonIsPressed(button, BUT_EXIT))
            TELEMETRY_MuteAlarm();
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            labelDesc.font = DEFAULT_FONT.font;  // set it back to 12x12 font
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        } else if (current_page != telemetry_off) {
            // this indicates whether telem is off or not supported
            if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
                _navigate_pages(1);
            } else if (CHAN_ButtonIsPressed(button, BUT_LEFT)) {
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
#endif //HAS_TELEMETRY
