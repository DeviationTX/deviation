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
static struct chantest_page * const cp = &pagemem.u.chantest_page;
static struct chantest_obj * const gui = &gui_objs.u.chantest;
static s32 showchan_cb(void *data);
static const char *value_cb(guiObject_t *obj, const void *data);
static void _handle_button_test();
static inline guiObject_t *_get_obj(int chan, int objid);
static int get_channel_idx(int chan);
static void _show_bar_page(int row);
static int cur_row = 0;

enum {
    ITEM_GRAPH,
    ITEM_VALUE,
    ITEM_GRAPH2,
    ITEM_VALUE2,
};

const char *lockstr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if(cp->is_locked == 1 || cp->is_locked == 2)
        return _tr("Touch to Unlock");
    else
        return _tr("Touch to Lock");
}

const char *button_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int button = (long)data;
    return INPUT_ButtonName(button + 1);
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if(cp->return_page) {
        PAGE_SetModal(0);
        PAGE_RemoveAllObjects();
        cp->return_page(cp->return_val);
    }
}

unsigned button_capture_cb(u32 button, unsigned flags, void *data)
{
    (void)button;
    (void)flags;
    (void)data;
    return 1;
}

void PAGE_ChantestEvent()
{
    if(cp->type == MONITOR_BUTTONTEST) {
        _handle_button_test();
        return;
    }
    volatile s32 *raw = MIXER_GetInputs();
    for(u32 i = 0; i < cp->num_bars; i++) {
        int ch = get_channel_idx(cur_row * NUM_BARS_PER_ROW + i);
        int v = RANGE_TO_PCT((ch >= NUM_INPUTS && ch < NUM_INPUTS + NUM_OUT_CHANNELS)
                             ? Channels[ch - NUM_INPUTS]
                             : raw[ch + 1]);
        if (v != cp->pctvalue[i]) {
            guiObject_t *obj = _get_obj(i, ITEM_VALUE);
            if (obj) {
                GUI_Redraw(obj);
                GUI_Redraw(_get_obj(i, ITEM_GRAPH));
            }
            cp->pctvalue[i] = v;
        }
    }
}

void PAGE_ChantestExit()
{
    BUTTON_UnregisterCallback(&cp->action);
}

static s32 showchan_cb(void *data)
{
    long ch = (long)data;
    return cp->pctvalue[ch];
}

static const char *value_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long ch = (long)data;
    sprintf(tempstring, "%d", cp->pctvalue[ch]);
    return tempstring;
}

static int get_channel_idx(int chan)
{
    int i;
    if (cp->type == MONITOR_MIXEROUTPUT) {
        for (i = 0; i < NUM_CHANNELS; i++) {
            if (Model.templates[i] != MIXERTEMPLATE_NONE) {
                if (--chan < 0)
                    break;
            }
        }
        return i + NUM_INPUTS;
    } else {
        if (cp->type == MONITOR_RAWINPUT) {
            int ppms = (PPMin_Mode() == PPM_IN_SOURCE) ? Model.num_ppmin & 0x3f: 0;
            if (chan < ppms)
                return NUM_INPUTS + NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS + chan;
            chan -= ppms;
        }
        for (i = 0; i < NUM_INPUTS; i++) {
            if (!(Transmitter.ignore_src & (1 << (i+1)))) {
                if (--chan < 0)
                    break;
            }
        }
        return i;
    }
}

static int num_disp_bars() {
    int j = 0;
    if (cp->type == MONITOR_MIXEROUTPUT) {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (Model.templates[i] != MIXERTEMPLATE_NONE) {
                j++;
            }
        }
    } else {
        for (int i = 0; i < NUM_INPUTS; i++) {
            if (!(Transmitter.ignore_src & (1 << (i+1)))) {
                j++;
            }
        }
        if (cp->type == MONITOR_RAWINPUT && PPMin_Mode() == PPM_IN_SOURCE) {
            j += Model.num_ppmin & 0x3f;
        }
    }
    return j;
}
