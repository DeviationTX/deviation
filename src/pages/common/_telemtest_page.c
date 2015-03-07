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

static struct telemtest_page * const tp  = &pagemem.u.telemtest_page;
static struct telemtest_obj  * const gui = &gui_objs.u.telemtest1;

#define TELEM_FONT NORMALBOX_FONT
#define TELEM_TXT_FONT DEFAULT_FONT
#define TELEM_ERR_FONT NORMALBOXNEG_FONT

static u8 telem_state_check()
{
    if (PAGE_TelemStateCheck(tempstring, sizeof(tempstring))==0) {
        memset(gui, 0, sizeof(*gui));
        return 0;
    }
    return 1;
}

static const char *telem_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    s32 val = (long)data;
    return TELEMETRY_GetValueStr(tempstring, val);
}

static const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long val = (long)data;
    TELEMETRY_ShortName(tempstring, val);
    strcpy(tempstring + strlen(tempstring), ":");
    return tempstring;
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if(tp->return_page) {
        PAGE_SetModal(0);
        PAGE_RemoveAllObjects();
        tp->return_page(tp->return_val);
    }
}

