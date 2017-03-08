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

#include "main_layout.h"

int elem_abs_to_rel(int idx)
{
    unsigned type = ELEM_TYPE(pc->elem[idx]);
    int nxt = -1;
    for (int i = 0; i < NUM_ELEMS-1; i++) {
        nxt = MAINPAGE_FindNextElem(type, nxt+1);
        if (nxt == idx)
            return i;
    }
    return 0;
}

int elem_rel_to_abs(int type, int idx)
{
    int nxt = -1;
    for(int i = 0; i < idx+1; i++)
        nxt = MAINPAGE_FindNextElem(type, nxt+1);
    return nxt;
}

int elem_get_count(int type)
{
    int nxt = -1;
    for (int i = 0; i < NUM_ELEMS; i++) {
        nxt = MAINPAGE_FindNextElem(type, nxt+1);
        if (nxt == -1)
            return i;
    }
    return 0;
}

const char *_GetBoxSource(char *str, int src, int real)
{
    if (src) {
#if HAS_RTC
        if (src <= NUM_RTC)
            return RTC_Name(str, src - 1);
#endif
        if (src - NUM_RTC <= NUM_TIMERS)
            return TIMER_Name(str, src - NUM_RTC - 1);
        else if( src - NUM_RTC - NUM_TIMERS <= NUM_TELEM)
            return TELEMETRY_Name(str, src - NUM_RTC - NUM_TIMERS);
    }
    if (real) {
        return INPUT_SourceNameReal(str, src
               ? src - (NUM_TELEM + NUM_TIMERS + NUM_RTC) + NUM_INPUTS
               : 0);
    } else {
        return INPUT_SourceName(str, src
               ? src - (NUM_TELEM + NUM_TIMERS + NUM_RTC) + NUM_INPUTS
               : 0);
    }
}

const char *GetBoxSource(char *str, int src)
{
    return _GetBoxSource(str, src, 0);
}

const char *GetBoxSourceReal(char *str, int src)
{
    return _GetBoxSource(str, src, 1);
}

const char *GetElemName(int type)
{
    switch(type) {
        case ELEM_SMALLBOX: return _tr("Small-box");
        case ELEM_BIGBOX:   return _tr("Big-box");
        case ELEM_TOGGLE:   return _tr("Toggle");
        case ELEM_BAR:      return _tr("Bargraph");
        case ELEM_VTRIM:    return _tr("V-trim");
        case ELEM_HTRIM:    return _tr("H-trim");
        case ELEM_MODELICO: return _tr("Model");
        case ELEM_BATTERY:  return _tr("Battery");
        case ELEM_TXPOWER:  return _tr("TxPower");
    }
    return "";
}

const char *newelem_cb(guiObject_t *obj, int dir, void *data)
{   
    (void)data;
    (void)obj;
    const int last_elem = (HAS_TOUCH) ? ELEM_BATTERY : ELEM_LAST; //FIXME
    lp->newelem = GUI_TextSelectHelper(lp->newelem, 0, last_elem-1, dir, 1, 1, NULL);
    return GetElemName(lp->newelem);
}

int create_element()
{
    int i;
    u16 x,y,w,h;
    for (i = 0; i < NUM_ELEMS; i++)
        if (! ELEM_USED(pc->elem[i]))
            break;
    if (i == NUM_ELEMS)
        return -1;
    y = 1;
    GetElementSize(lp->newelem, &w, &h);
    x = (LCD_WIDTH - w) / 2;
    y = (((LCD_HEIGHT - HEADER_Y) - h) / 2) + HEADER_Y;
    memset(&pc->elem[i], 0, sizeof(struct elem));
    ELEM_SET_X(pc->elem[i], x);
    ELEM_SET_Y(pc->elem[i], y);
    ELEM_SET_TYPE(pc->elem[i], lp->newelem);
    return i;
}

static const char *add_dlgbut_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return data ? _tr("Add") : _tr("Load");
}

static int _adjust_src_for_telemetry(int src, int dir)
{
    if (src > NUM_RTC + NUM_TIMERS && src <= NUM_RTC + NUM_TIMERS + NUM_TELEM) {
        if (PROTOCOL_GetTelemetryState() != PROTO_TELEM_ON) {
            //We chose a telemetry item, but there is no telemetry
            src = (dir > 0) ? NUM_RTC + NUM_TIMERS + NUM_TELEM + 1 : NUM_RTC + NUM_TIMERS;
        } else {
            int max = TELEMETRY_GetNumTelemSrc();
            if (src > NUM_RTC + NUM_TIMERS + max) {
                src = (dir > 0) ? NUM_RTC + NUM_TIMERS + NUM_TELEM + 1 : NUM_RTC + NUM_TIMERS + max;
            }
        }
    }
    return src;
}

static const char *dlgts_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    int type = ELEM_TYPE(pc->elem[idx]);
    switch (type) {
        case ELEM_SMALLBOX:
        case ELEM_BIGBOX:
        {
            u8 changed = 0;
            pc->elem[idx].src = GUI_TextSelectHelper(pc->elem[idx].src, 0, NUM_RTC + NUM_TELEM + NUM_TIMERS + NUM_CHANNELS, dir, 1, 1, &changed);
            if(changed && dir) {
                pc->elem[idx].src = _adjust_src_for_telemetry(pc->elem[idx].src, dir);
            }
            return GetBoxSource(tempstring, pc->elem[idx].src);
        }
        case ELEM_BAR:
            pc->elem[idx].src = GUI_TextSelectHelper(pc->elem[idx].src, 0, NUM_CHANNELS, dir, 1, 1, NULL);
            return INPUT_SourceName(tempstring, pc->elem[idx].src ? pc->elem[idx].src + NUM_INPUTS : 0);
        case ELEM_TOGGLE:
        {
            pc->elem[idx].src = INPUT_SelectAbbrevSource(pc->elem[idx].src, dir);
            return INPUT_SourceNameAbbrevSwitch(tempstring, pc->elem[idx].src);
        }
        case ELEM_HTRIM:
        case ELEM_VTRIM:
            pc->elem[idx].src = GUI_TextSelectHelper(pc->elem[idx].src, 0, NUM_TRIMS, dir, 1, 1, NULL);
            if (pc->elem[idx].src == 0)
                return _tr("None");
            snprintf(tempstring, sizeof(tempstring), "%s%d", _tr("Trim"),pc->elem[idx].src);
            return tempstring;
    }
    return _tr("None");
}

static void dlgbut_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    int i;
    //Remove object
    int type = ELEM_TYPE(pc->elem[idx]);
    for(i = idx+1; i < NUM_ELEMS; i++) {
        if (! ELEM_USED(pc->elem[i]))
            break;
        pc->elem[i-1] = pc->elem[i];
    }
         ELEM_SET_X(pc->elem[i-1], 0);
         ELEM_SET_Y(pc->elem[i-1], 0);
    idx = MAINPAGE_FindNextElem(type, 0);
    set_selected_for_move(idx);
    //close the dialog and reopen with new elements
    show_config();
}

const char *menulabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long i = (long)data;
    snprintf(tempstring, sizeof(tempstring), "%s %d", _tr("Menu"), (int)i+1);
    return tempstring;
}

const char *menusel_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    (void) dir;
    int i = (long)data;
    int max_pages = PAGE_GetNumPages();
    int start_page = PAGE_GetStartPage();
    int page = GUI_TextSelectHelper(pc->quickpage[i], start_page, max_pages -1, dir, 1, 1, NULL);
    if (page != pc->quickpage[i]) {
        int increment = (page > pc->quickpage[i]) ? 1 : -1;
        while (page >= start_page && page != max_pages && ! PAGE_IsValidQuickPage(page)) {
            page = (page + increment);
        }
        if (page >= start_page && page != max_pages)
            pc->quickpage[i] = page;
    }
    return PAGE_GetName(pc->quickpage[i]);
}
