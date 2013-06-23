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
    unsigned type = ELEM_TYPE(pc.elem[idx]);
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

const char *GetBoxSource(char *str, int src)
{
    if (src) {
        if (src <= NUM_TIMERS)
            return TIMER_Name(str, src - 1);
        else if( src - NUM_TIMERS <= NUM_TELEM)
        return TELEMETRY_Name(str, src - NUM_TIMERS);
    }
    return INPUT_SourceName(str, src
               ? src - (NUM_TELEM + NUM_TIMERS) + NUM_INPUTS
               : 0);
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
    }
    return "";
}

const char *newelem_cb(guiObject_t *obj, int dir, void *data)
{   
    (void)data;
    (void)obj;
    lp.newelem = GUI_TextSelectHelper(lp.newelem, 0, ELEM_LAST-1, dir, 1, 1, NULL);
    return GetElemName(lp.newelem);
}

int create_element()
{
    int i;
    u16 x,y,w,h;
    for (i = 0; i < NUM_ELEMS; i++)
        if (! ELEM_USED(pc.elem[i]))
            break;
    if (i == NUM_ELEMS)
        return -1;
    y = 1;
    GetElementSize(lp.newelem, &w, &h);
    x = (LCD_WIDTH - w) / 2;
    y = (((LCD_HEIGHT - HEADER_Y) - h) / 2) + HEADER_Y;
    memset(&pc.elem[i], 0, sizeof(struct elem));
    ELEM_SET_X(pc.elem[i], x);
    ELEM_SET_Y(pc.elem[i], y);
    ELEM_SET_TYPE(pc.elem[i], lp.newelem);
    return i;
}

static const char *add_dlgbut_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return data ? _tr("Add") : _tr("Load");
}

static const char *dlgts_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    int type = ELEM_TYPE(pc.elem[idx]);
    switch (type) {
        case ELEM_SMALLBOX:
        case ELEM_BIGBOX:
        {
            pc.elem[idx].src = GUI_TextSelectHelper(pc.elem[idx].src, 0, NUM_TELEM + NUM_TIMERS + NUM_CHANNELS, dir, 1, 1, NULL);   
            return GetBoxSource(lp.tmp, pc.elem[idx].src);
        }
        case ELEM_BAR:
            pc.elem[idx].src = GUI_TextSelectHelper(pc.elem[idx].src, 0, NUM_CHANNELS, dir, 1, 1, NULL);   
            return INPUT_SourceName(lp.tmp, pc.elem[idx].src ? pc.elem[idx].src + NUM_INPUTS : 0);
        case ELEM_TOGGLE:
        {
            int val = MIXER_SRC(pc.elem[idx].src);
            int newval = GUI_TextSelectHelper(val, 0, NUM_SOURCES, dir, 1, 1, NULL);
            newval = INPUT_GetAbbrevSource(val, newval, dir);
            if (val != newval) {
                val = newval;
                pc.elem[idx].src = val;
            }
            return INPUT_SourceNameAbbrevSwitch(lp.tmp, pc.elem[idx].src);
        }
        case ELEM_HTRIM:
        case ELEM_VTRIM:
            pc.elem[idx].src = GUI_TextSelectHelper(pc.elem[idx].src, 0, NUM_TRIMS, dir, 1, 1, NULL);
            sprintf(lp.tmp, "%s%d", _tr("Trim"),pc.elem[idx].src + 1);
            return lp.tmp;
    }
    return "";
}

static void dlgbut_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    int i;
    //Remove object
    int type = ELEM_TYPE(pc.elem[idx]);
    for(i = idx+1; i < NUM_ELEMS; i++) {
        if (! ELEM_USED(pc.elem[i]))
            break;
        pc.elem[i-1] = pc.elem[i];
    }
         ELEM_SET_Y(pc.elem[i-1], 0);
    idx = MAINPAGE_FindNextElem(type, 0);
    set_selected_for_move(idx);
    //close the dialog and reopen with new elements
    show_config();
}

static void add_dlgbut_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    if(data) {
    } else {
        PAGE_MainLayoutExit();
        MODELPage_ShowLoadSave(LOAD_LAYOUT, PAGE_MainLayoutInit);
    }
}

