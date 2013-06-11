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

static struct trim_page * const tp = &pagemem.u.trim_page;
static void _show_page();
static void _edit_cb(guiObject_t *obj, const void *data);

const char *trimsource_name_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 i = (long)data;
    struct Trim *trim = MIXER_GetAllTrims();
    return INPUT_SourceName(tp->tmpstr, MIXER_MapChannel(trim[i].src));
}

static const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    *source = GUI_TextSelectHelper(MIXER_SRC(*source), 0, NUM_SOURCES, dir, 1, 1, NULL);
    return INPUT_SourceName(tp->tmpstr, MIXER_MapChannel(*source));
}

static const char *set_switch_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 *source = (u8 *)data;
    u8 changed;
    u8 val = MIXER_SRC(*source);

    int newval = GUI_TextSelectHelper(val, 0, NUM_SOURCES, dir, 1, 1, &changed);
    newval = INPUT_GetAbbrevSource(val, newval, dir);
    if (val != newval) {
        val = newval;
        *source = val;
    }
    return INPUT_SourceNameAbbrevSwitch(tp->tmpstr, *source);
}

const char *set_trim_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *button = (u8 *)data;
    *button = GUI_TextSelectHelper(*button, 0, NUM_TX_BUTTONS, dir, 1, 1, NULL);
    return INPUT_ButtonName(*button);
}

static const char *set_trimstep_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    s8 *value = (s8 *)data;
    *value = GUI_TextSelectHelper(*value, -100, 100, dir, 1, 5, NULL);
    s8 val = *value < 0 ? -*value : *value;
    sprintf(tp->tmpstr, "%s%d.%d", *value < 0 ? "-" : "",val / 10, val % 10);
    return tp->tmpstr;
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    if (data) {
        //Save trim here
        struct Trim *trim = MIXER_GetAllTrims();
        trim[tp->index] = tp->trim;
        MIXER_RegisterTrimButtons();
    }
    PAGE_RemoveAllObjects();
    PAGE_TrimInit(0);
}

void PAGE_TrimInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();

    _show_page();
}

void PAGE_TrimEvent()
{
}

