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


static guiObject_t *dialog = NULL;
static guiObject_t *current_selected_obj = NULL; // used for devo10 only

/******************/
/*  Safety Dialog */
/******************/
static u64 safety_enabled = ~0LL;

static u64 safety_check()
{
    return PROTOCOL_CheckSafe() & safety_enabled;
}

static void safety_confirmed()
{
    PROTOCOL_Init(1);
    safety_enabled = ~0LL;
}

static void safety_ok_cb(u8 state, void * data)
{
    (void)state;
    (void)data;
    int count = 0;
    u64 unsafe = safety_check();
    for(int i = 0; i < NUM_SOURCES + 1; i++) {
        if (! (unsafe & (1LL << i)))
            continue;
        safety_enabled ^= 1LL << i;
        if (++count >= MAX_CONCURRENT_SAFETY_MSGS)
            break;
    }
    dialog = NULL;
    if (current_selected_obj != NULL)
        GUI_SetSelected(current_selected_obj);
    if (unsafe & safety_enabled)
        return;
    safety_confirmed();
}

/**********************/
/* Low battery Dialog */
/**********************/
static void lowbatt_ok_cb(u8 state, void * data)
{
    (void)state;
    (void)data;
    if (current_selected_obj != NULL)
        GUI_SetSelected(current_selected_obj);
    dialog = NULL;
}


static void invalid_stdmixer_cb(u8 state, void *guiObj)
{
    static const char * const STANDARD_TEMPLATE = "heli_std.ini";
    if (current_selected_obj != NULL)
        GUI_SetSelected(current_selected_obj);
    dialog = NULL;
    if (state == 1) {
        PAGE_RemoveHeader();
        memset(Model.mixers, 0, sizeof(Model.mixers)); // // reset all mixers first
        memset(Model.limits, 0, sizeof(Model.limits)); // // reset all mixers first
        CONFIG_ReadTemplate(STANDARD_TEMPLATE); // load template
        if (guiObj != NULL) {
            GUI_Redraw(guiObj);
        }
    }
}

static void reset_permtimer_cb(u8 state, void *data)
{
    u8 idx = (long)data;
    if (current_selected_obj != NULL)
	GUI_SetSelected(current_selected_obj);
    dialog = NULL;
    if (state == 1) {
	Model.timer[idx].val = 0;
	TIMER_Init();
    }
}

const char *reset_timer_string_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    u8 idx = (long)data;
    snprintf(tempstring, sizeof(tempstring), _tr("Do you really want\nto reset the\n permanent timer %d?"),idx+1);
    return tempstring;
}

int PAGE_DialogVisible()
{
   return dialog ? 1 : 0;
}
