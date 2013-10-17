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

static struct main_page * const mp = &pagemem.u.main_page;
#define gui (&gui_objs.u.mainpage)
#define pc Model.pagecfg2
const char *show_box_cb(guiObject_t *obj, const void *data);
const char *voltage_cb(guiObject_t *obj, const void *data);
static s16 trim_cb(void * data);
static s16 bar_cb(void * data);
void press_icon2_cb(guiObject_t *obj, const void *data);
static u8 _action_cb(u32 button, u8 flags, void *data);
static s32 get_boxval(u8 idx);
static void _check_voltage();

struct ImageMap TGLICO_GetImage(int idx);

struct LabelDesc *get_box_font(u8 idx, u8 neg)
{
    if(neg) {
        return idx & 0x02 ? &SMALLBOXNEG_FONT : &BIGBOXNEG_FONT;
    } else {
        return idx & 0x02 ? &SMALLBOX_FONT : &BIGBOX_FONT;
    }
}

s32 get_boxval(u8 idx)
{
#if HAS_RTC
    if (idx <= NUM_RTC) {
        u32 time = RTC_GetValue();
        return idx == 1 ? RTC_GetTimeValue(time) : RTC_GetDateValue(time);
    }
#endif
    if (idx - NUM_RTC <= NUM_TIMERS)
        return TIMER_GetValue(idx - NUM_RTC - 1);
    if(idx - NUM_RTC - NUM_TIMERS <= NUM_TELEM)
        return TELEMETRY_GetValue(idx - NUM_RTC - NUM_TIMERS);
    return RANGE_TO_PCT(MIXER_GetChannel(idx - (NUM_RTC + NUM_TIMERS + NUM_TELEM + 1), APPLY_SAFETY | APPLY_SCALAR));
}

const char *show_box_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
#if HAS_RTC
    if (idx <= NUM_RTC) {
        u32 time = RTC_GetValue();
        idx == 1 ? RTC_GetTimeString(mp->tmpstr, time) : RTC_GetDateString(mp->tmpstr, time);
        return mp->tmpstr;
    }
#endif
    if (idx - NUM_RTC <= NUM_TIMERS) {
        TIMER_SetString(mp->tmpstr, TIMER_GetValue(idx - NUM_RTC - 1));
    } else if(idx - NUM_RTC - NUM_TIMERS <= NUM_TELEM) {
        TELEMETRY_GetValueStr(mp->tmpstr, idx - NUM_RTC - NUM_TIMERS);
    } else {
        sprintf(mp->tmpstr, "%3d%%", RANGE_TO_PCT(MIXER_GetChannel(idx - (NUM_RTC + NUM_TIMERS + NUM_TELEM + 1), APPLY_SAFETY | APPLY_SCALAR)));
    }
    return mp->tmpstr;
}

const char *voltage_cb(guiObject_t *obj, const void *data) {
    (void)obj;
    (void)data;
    if (mp->battery > 1000)  // bug fix: any value lower than 1v means the DMA reading is not ready
        sprintf(mp->tmpstr, "%2d.%02dV", mp->battery / 1000, (mp->battery % 1000) / 10);
    else
        mp->tmpstr[0] = 0;
    return mp->tmpstr;
}

#if HAS_RTC
static const char *time_cb(guiObject_t *obj, const void *data) {
    (void)obj;
    (void)data;
    RTC_GetTimeStringShort(mp->tmpstr, RTC_GetValue());
    return mp->tmpstr;
}
#endif

s16 trim_cb(void * data)
{
    long i = (long)data;
    int value = *MIXER_GetTrim(i);
    return PCT_TO_RANGE(value);
}

s16 bar_cb(void * data)
{
    u8 idx = (long)data;
    return MIXER_GetChannel(idx-1, APPLY_SAFETY);
}

void PAGE_MainEvent()
{
    int i;
    if (PAGE_GetModal()) {
        if(pagemem.modal_page == 2) {
            PAGE_TelemtestEvent();
        }
        return;
    }
    volatile s16 *raw = MIXER_GetInputs();
    for(i = 0; i < NUM_ELEMS; i++) {
        if (! ELEM_USED(pc.elem[i]))
            break;
        if (! OBJ_IS_USED(&gui->elem[i]))
            continue;
        int src = pc.elem[i].src;
        int type = ELEM_TYPE(pc.elem[i]);
        switch(type) {
            case ELEM_VTRIM:
            case ELEM_HTRIM:
            {
                int value = *(MIXER_GetTrim(src-1));
                if (mp->elem[i] != value) {
                    mp->elem[i] = value;
                    GUI_Redraw(&gui->elem[i].bar);
                }
                break;
            }
            case ELEM_SMALLBOX:
            case ELEM_BIGBOX:
            {
                s32 val = get_boxval(src);
#if HAS_RTC
                if (src <= NUM_RTC) {
                    if (mp->elem[i] != val) {
                        mp->elem[i] = val;
                        GUI_Redraw(&gui->elem[i].box);
                    }
                } else
#endif
                if (src - NUM_RTC <= NUM_TIMERS) {
                    //Timer
                    if ((val >= 0 && mp->elem[i] < 0) || (val < 0 && mp->elem[i] >= 0)) {
                        GUI_SetLabelDesc(&gui->elem[i].box, get_box_font(type == ELEM_BIGBOX ? 0 : 2, val < 0));
                        mp->elem[i] = val;
                        GUI_Redraw(&gui->elem[i].box);
                    } else if (mp->elem[i] / 1000 != val /1000) {
                        mp->elem[i] = val;
                        GUI_Redraw(&gui->elem[i].box);
                    }
                } else if (src - NUM_RTC - NUM_TIMERS <= NUM_TELEM) {
                    //Telem
                    int alarm = TELEMETRY_HasAlarm(src - NUM_RTC - NUM_TIMERS);
                    if (alarm || ! TELEMETRY_IsUpdated(0xff)) {
                        GUI_SetLabelDesc(&gui->elem[i].box, get_box_font(type == ELEM_BIGBOX ? 0 : 2, 1));
                    } else if(mp->elem[i] != val) {
                        GUI_SetLabelDesc(&gui->elem[i].box, get_box_font(type == ELEM_BIGBOX ? 0 : 2, 0));
                        mp->elem[i] = val;
                        GUI_Redraw(&gui->elem[i].box);
                    }
                } else if (mp->elem[i] != val) {
                    //Source
                    mp->elem[i] = val;
                    GUI_Redraw(&gui->elem[i].box);
                }
                break;
            }
            case ELEM_BAR:
            {
                s16 chan = MIXER_GetChannel(src-1, APPLY_SAFETY);
                if (mp->elem[i] != chan) {
                    mp->elem[i] = chan;
                    GUI_Redraw(&gui->elem[i].bar);
                }
                break;
            }
            case ELEM_TOGGLE:
            {
                src = MIXER_SRC(src);
                struct ImageMap img;
                (void)img.x_off;
                (void)img.y_off;
                img.file = NULL;
                if (src > INP_HAS_CALIBRATION && src < INP_LAST) {
                    //switch
                    for (int j = 0; j < 3; j++) {
                        // Assume switch 0/1/2 are in order
                        if(ELEM_ICO(pc.elem[i], j) && raw[src+j] > 0) {
                            img = TGLICO_GetImage(ELEM_ICO(pc.elem[i], j));
                            break;
                        }
                    }
                } else {
                    //Non switch
                    int sw = raw[src] > 0 ? 1 : 0;
                    if (ELEM_ICO(pc.elem[i], sw)) {
                        img = TGLICO_GetImage(ELEM_ICO(pc.elem[i], sw));
                    }
                }
                if (img.file) {
                    GUI_ChangeImage(&gui->elem[i].img, img.file, img.x_off, img.y_off);
                    GUI_SetHidden((guiObject_t *)&gui->elem[i].img, 0);
                } else {
                    GUI_SetHidden((guiObject_t *)&gui->elem[i].img, 1);
                }
            }
            break;
        }
    }
    _check_voltage();
#if HAS_RTC
    if(Display.flags & SHOW_TIME) {
        u32 time = RTC_GetValue() / 60;
        if(mp->time != time) {
            mp->time = time;
            GUI_Redraw(&gui->time);
        }
    }
#endif
}
void GetElementSize(unsigned type, u16 *w, u16 *h)
{
    const u8 width[ELEM_LAST] = {
        [ELEM_SMALLBOX] = BOX_W,
        [ELEM_BIGBOX]   = BOX_W,
        [ELEM_TOGGLE]   = TOGGLEICON_WIDTH,
        [ELEM_BAR]      = GRAPH_W,
        [ELEM_VTRIM]    = VTRIM_W,
        [ELEM_HTRIM]    = HTRIM_W,
        [ELEM_MODELICO] = MODEL_ICO_W,
    };
    const u8 height[ELEM_LAST] = {
        [ELEM_SMALLBOX] = SMALLBOX_H,
        [ELEM_BIGBOX]   = BIGBOX_H,
        [ELEM_TOGGLE]   = TOGGLEICON_HEIGHT,
        [ELEM_BAR]      = GRAPH_H,
        [ELEM_VTRIM]    = VTRIM_H,
        [ELEM_HTRIM]    = HTRIM_H,
        [ELEM_MODELICO] = MODEL_ICO_H,
    };
    *w = width[type];
    *h = height[type];
}
int GetWidgetLoc(struct elem *elem, u16 *x, u16 *y, u16 *w, u16 *h)
{
    *y = ELEM_Y(*elem);
    if (*y == 0)
        return 0;
    int type = ELEM_TYPE(*elem);
    if (type >= ELEM_LAST)
        return 0;
    *x = ELEM_X(*elem);
    GetElementSize(type, w, h);
    return 1;
}

unsigned map_type(int type)
{
    switch(type) {
        case ELEM_BIGBOX: return ELEM_SMALLBOX;
        case ELEM_HTRIM: return ELEM_VTRIM;
        default: return type;
    }
}
int MAINPAGE_FindNextElem(unsigned type, int idx)
{
    type = map_type(type);
    for(int i = idx; i < NUM_ELEMS; i++) {
        if(! ELEM_USED(pc.elem[i]))
            break;
        if (map_type(ELEM_TYPE(pc.elem[i])) == type)
            return i;
    }
    return -1;
}

void show_elements()
{
    u16 x, y, w, h;
    for (int i = 0; i < NUM_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.elem[i], &x, &y, &w, &h))
            break;
        int type = ELEM_TYPE(pc.elem[i]);
        switch(type) {
            case ELEM_MODELICO:
                GUI_CreateImageOffset(&gui->elem[i].img, x, y, w, h, 0, 0, CONFIG_GetCurrentIcon(), press_icon_cb, (void *)1);
                break;
            case ELEM_VTRIM:
            case ELEM_HTRIM:
            {
                int src = pc.elem[i].src;
                if (src == 0)
                    continue;
                mp->elem[i] = *(MIXER_GetTrim(src-1));
                GUI_CreateBarGraph(&gui->elem[i].bar, x, y, w, h, -10000, 10000,
                    type == ELEM_VTRIM ? TRIM_VERTICAL : TRIM_INVHORIZONTAL, trim_cb, (void *)(long)(src-1));
                break;
            }
            case ELEM_SMALLBOX:
            case ELEM_BIGBOX:
            {
                int src = pc.elem[i].src;
                if (src == 0)
                    continue;
                mp->elem[i] = get_boxval(src);
                int font = (
#if HAS_RTC
                           src > NUM_RTC &&
#endif
                           ((src <= NUM_RTC + NUM_TIMERS && mp->elem[i] < 0)
//                           || ((u8)(src - NUM_RTC - NUM_TIMERS - 1) < NUM_TELEM && Telemetry.time[0] == 0)
                           ));
                GUI_CreateLabelBox(&gui->elem[i].box, x, y, w, h,
                            get_box_font(type == ELEM_BIGBOX ? 0 : 2, font),
                            show_box_cb, press_box_cb,
                            (void *)((long)src));
                break;
            }
            case ELEM_BAR:
            {
                int src = pc.elem[i].src;
                if (src == 0)
                    continue;
                mp->elem[i] = MIXER_GetChannel(src-1, APPLY_SAFETY);
                GUI_CreateBarGraph(&gui->elem[i].bar, x, y, w, h, CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL,
                           bar_cb, (void *)((long)src));
                break;
            }
            case ELEM_TOGGLE:
            {
                struct ImageMap img = TGLICO_GetImage(ELEM_ICO(pc.elem[i], 0)); //We'll set this properly down below
                GUI_CreateImageOffset(&gui->elem[i].img, x, y, w, h,
                                  img.x_off, img.y_off, img.file, NULL, NULL);
                break;
            }
        }
    }
}
