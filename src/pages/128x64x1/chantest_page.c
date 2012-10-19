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
#include "gui/gui.h"
#include "config/model.h"

static struct chantest_page * const cp = &pagemem.u.chantest_page;

static s16 showchan_cb(void *data);
static const char *value_cb(guiObject_t *obj, const void *data);
//static const char *channum_cb(guiObject_t *obj, const void *data);

static u8 action_cb(u32 button, u8 flags, void *data)
{
	(void)data;
	if (flags & BUTTON_PRESS) {
		if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
			PAGE_ChangeByName("MainMenu", 0);
		} else {
			// only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
			return 0;
		}
	}
	return 1;
}

static void show_bar_page(u8 num_bars)
{
    #define SEPERATION
    int i;
    u8 height;
    u8 count;
    if (num_bars > 12)
        num_bars = 12;
    cp->num_bars = num_bars;
    height = 4;
    u8 width = 59; // better to be even
    count = num_bars;
    u16 offset = 0;
    u8 separation = (LCD_HEIGHT - offset*2)*2/ count;
    u8 x, y;
    memset(cp->pctvalue, 0, sizeof(cp->pctvalue));
    y = offset;
    for(i = 0; i < count; i++) {
    	if (i%2 ==0) {
    		x = 1;
    		y += separation -1;
    	} else {
    		x = 65;
    	}
    	//GUI_CreateLabelBox(x + 25, y - 8,
    	//			0, 0, &TINY_FONT, channum_cb, NULL, (void *)(long)i);
		cp->bar[i] = GUI_CreateBarGraph(x , y, width, height,
									-125, 125, TRIM_HORIZONTAL,
									showchan_cb,
									(void *)((long)i));
		cp->value[i] = GUI_CreateLabelBox(x + 10, y- 7,
		                              32, 7, &TINY_FONT, value_cb, NULL, (void *)((long)i));
    }
}

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

static void show_button_page()
{
    #define X_STEP 95
    int i;
    cp->is_locked = 3;
    int y = 64;
    cp->bar[0] = GUI_CreateLabelBox(100, 40, 0, 0, &DEFAULT_FONT, lockstr_cb, NULL, NULL);
    for (i = 0; i < NUM_TX_BUTTONS; i++) {
        GUI_CreateLabelBox(10 + X_STEP * (i % 3), y, 0, 0,
                         &DEFAULT_FONT, button_str_cb, NULL, (void *)(long)i);
        cp->value[i] = GUI_CreateLabelBox(70 + X_STEP * (i % 3), y, 16, 16,
                         &SMALLBOX_FONT, NULL, NULL, (void *)"");
        if ((i % 3) == 2)
            y += 24;
    }
}

void PAGE_ChantestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    BUTTON_RegisterCallback(&cp->action,
          CHAN_ButtonMask(BUT_EXIT),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE | BUTTON_PRIORITY, action_cb, NULL);
    cp->return_page = NULL;
    cp->type = 0;
	// Todo: handle over 12 channels in 2 pages
	u8 channel_no = Model.num_channels;
	if (channel_no > 10) channel_no = 10;
    show_bar_page(channel_no); //Model.num_channels);
}

void PAGE_InputtestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(_tr("Inputs"));
    cp->return_page = NULL;
    cp->type = 1;
    show_bar_page(NUM_INPUTS);
}

void PAGE_ButtontestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(_tr("Buttons"));
    cp->return_page = NULL;
    cp->type = 2;
    show_button_page();
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if(cp->return_page) {
        PAGE_SetModal(0);
        PAGE_RemoveAllObjects();
        cp->return_page(1);
    }
}

void PAGE_ChantestModal(void(*return_page)(int page))
{
    PAGE_SetModal(1);
    cp->return_page = return_page;
    cp->type = 0;
    PAGE_RemoveAllObjects();

    PAGE_ShowHeader_ExitOnly(_tr("Channels"), okcancel_cb);

    show_bar_page(Model.num_channels);
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
    s16 *raw = MIXER_GetInputs();
    for(i = 0; i < cp->num_bars; i++) {
        int v = RANGE_TO_PCT(cp->type ? raw[i+1] : Channels[i]);
        if (v != cp->pctvalue[i]) {
            GUI_Redraw(cp->bar[i]);
            GUI_Redraw(cp->value[i]);
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

/* static const char *channum_cb(guiObject_t *obj, const void *data)
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
} */
