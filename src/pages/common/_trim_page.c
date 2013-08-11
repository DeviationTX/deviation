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

enum {
    TRIM_MINUS,
    TRIM_SWITCH,
};

static inline guiObject_t * _get_obj(int idx, int objid);
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
    if(! GUI_IsTextSelectEnabled(obj)){
        return _tr("None");
    }
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
    if(! GUI_IsTextSelectEnabled(obj)){
        return _tr("None");
    }
    u8 *button = (u8 *)data;
    *button = GUI_TextSelectHelper(*button, 0, NUM_TX_BUTTONS, dir, 1, 1, NULL);
    return INPUT_ButtonName(*button);
}

static const char *set_trimstep_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int i = (long)data;
    u8 *value = &Model.trims[i].step;
    //place switches before 0.1 on the spinbox
    u8 v = ((int)*value + TRIM_SWITCH_TYPES - 1) % (190 + TRIM_SWITCH_TYPES);
    v = GUI_TextSelectHelper(v, 0, 190 + TRIM_SWITCH_TYPES - 1, dir, 1, 5, NULL);
    *value = ((int)v + 190) % (190 + TRIM_SWITCH_TYPES) + 1;

    guiObject_t *negtrimObj = _get_obj(i, TRIM_MINUS);
    guiObject_t *switchObj  = _get_obj(i, TRIM_SWITCH);

    int hide_trim = 0;
    int hide_switch = 0;
    if (*value == TRIM_MOMENTARY) {
        strcpy(tp->tmpstr, _tr("Momentary"));
        hide_trim = 1; hide_switch = 1;
    } else if (*value == TRIM_TOGGLE) {
        strcpy(tp->tmpstr, _tr("Toggle"));
        hide_trim = 1; hide_switch = 1;
    } else if (*value == TRIM_ONOFF) {
        strcpy(tp->tmpstr, _tr("On/Off"));
        hide_switch = 1;
    } else if (*value < 100) {
        sprintf(tp->tmpstr, "%d.%d", *value / 10, *value % 10);
    } else {
        sprintf(tp->tmpstr, "%d", *value - 90);
    }
    if (negtrimObj) {
        if (negtrimObj->Type == TextSelect)
            GUI_TextSelectEnable((guiTextSelect_t *)negtrimObj, ! hide_trim);
        else
            GUI_Redraw(negtrimObj);
    }
    if (switchObj)
        GUI_TextSelectEnable((guiTextSelect_t *)switchObj, ! hide_switch);
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

