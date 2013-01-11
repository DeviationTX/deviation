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
#define gui (&gui_objs.u.chantest)
static s16 showchan_cb(void *data);
static const char *value_cb(guiObject_t *obj, const void *data);
static const char *channum_cb(guiObject_t *obj, const void *data);
static void _show_bar_page(u8 num_bars);
static void _handle_button_test();

enum {
    ITEM_GRAPH,
    ITEM_VALUE,
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

u8 button_capture_cb(u32 button, u8 flags, void *data)
{
    (void)button;
    (void)flags;
    (void)data;
    return 1;
}

void PAGE_ChantestEvent()
{
    int i;
    if(cp->type == MONITOR_BUTTONTEST) {
        _handle_button_test();
        return;
    }
    volatile s16 *raw = MIXER_GetInputs();
    for(i = 0; i < cp->num_bars; i++) {
        int v = RANGE_TO_PCT(cp->type ? raw[i+1] : Channels[i]);
        if (v != cp->pctvalue[i]) {
            guiObject_t *obj = GUI_GetScrollableObj(&gui->scrollable, i, ITEM_GRAPH);
            if (obj) {
                GUI_Redraw(obj);
                GUI_Redraw(GUI_GetScrollableObj(&gui->scrollable, i, ITEM_VALUE));
            }
            cp->pctvalue[i] = v;
        }
    }
}

void PAGE_ChantestExit()
{
    BUTTON_UnregisterCallback(&cp->action);
}
static s16 showchan_cb(void *data)
{
    long ch = (long)data;
    return cp->pctvalue[ch];
}

static const char *value_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long ch = (long)data;
    sprintf(cp->tmpstr, "%d", cp->pctvalue[ch]);
    return cp->tmpstr;
}

static const char *channum_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long ch = (long)data;
    if (cp->type) {
        char *p = cp->tmpstr;
        if (ch & 0x01) {
            *p = '\n';
            p++;
        }
        INPUT_SourceName(p, ch+1);
        if (! (ch & 0x01)) {
            sprintf(p + strlen(p), "\n");
        }
    } else {
       sprintf(cp->tmpstr, "\n%d", (int)ch+1);
    }
    return cp->tmpstr;
}
