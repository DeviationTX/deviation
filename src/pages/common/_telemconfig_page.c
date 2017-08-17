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

static struct telemcfg_obj     * const gui = &gui_objs.u.telemcfg;
static struct telemconfig_page * const tp  = &pagemem.u.telemconfig_page;

static inline guiObject_t *_get_obj(int idx, int objid);

static u8 telem_state_check()
{
    if (PAGE_TelemStateCheck(tempstring, sizeof(tempstring))==0) {
        return 0;
    }
    return 1;
}

static const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    snprintf(tempstring, sizeof(tempstring), "%s%d", _tr("Alarm"), (int)((long)data)+1);
    return tempstring;
}

static const char *telem_name_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int val = (long)data;
    int telem_idx = Model.telem_alarm[val];
    int last = TELEMETRY_GetNumTelemSrc();
    u8 changed;
    //skip over (don't allow selection of) telemetry src's with max=0 (eg. JETCAT_STATUS, JETCAT_OFFCOND)
    while (1) {
        telem_idx = GUI_TextSelectHelper(telem_idx, 0, last, dir, 1, 1, &changed);
        if (telem_idx == 0 || TELEMETRY_GetMaxValue(telem_idx))
            break;
        if (telem_idx == last)
            dir = -1;
    }
    Model.telem_alarm[val] = telem_idx;
    if (changed) {
        guiObject_t *valObj = _get_obj(val, 2);
        if (valObj)
            GUI_Redraw(valObj);
    }
    return TELEMETRY_ShortName(tempstring, Model.telem_alarm[val]);
}

static const char *gtlt_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int val = (long)data;
    u8 changed;
    u8 type = ((Model.telem_flags >> val) & 1);
    type = GUI_TextSelectHelper(type, 0, 1, -dir, 1, 1, &changed);
    if (changed) {
        if (type) {
            Model.telem_flags |= 1 << val;
        } else {
            Model.telem_flags &= ~(1 << val);
        }
        TELEMETRY_ResetAlarm(val);
    }
    return type ? "<=" : ">";
}

static const char *limit_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    u8 telem_idx = Model.telem_alarm[idx];
    if (telem_idx == 0) {
        return "----";
    }
    s32 min = TELEMETRY_GetMinValue(telem_idx);
    s32 max = TELEMETRY_GetMaxValue(telem_idx);
    s32 value = Model.telem_alarm_val[idx];

    u32 small_step = 1;
    u32 big_step = 10;
    if (value > 1000 || (value == 1000 && dir > 0)) {
        small_step = 10;
        big_step = 100;
    }
    if (min >= 50) {  //RPM
        small_step = 50;
        big_step = 500;
    } else if (max > 9999) {
        if (value > 10000 || (value == 10000 && dir > 0)) {
            small_step = 100;
            big_step = 1000;
        }
    } else if (max > 999) {
        small_step = 10;
        big_step = 100;
    }

    Model.telem_alarm_val[idx] = GUI_TextSelectHelper(value, min, max, dir, small_step, big_step, NULL);
    TELEMETRY_GetValueStrByValue(tempstring, telem_idx, Model.telem_alarm_val[idx]);
    if (Model.telem_alarm_th[idx] > 0) {
        char tmpstr[sizeof(tempstring)];
        strlcpy(tmpstr, tempstring, sizeof(tmpstr));
        snprintf(tempstring,sizeof(tempstring),"%ds%s%s",Model.telem_alarm_th[idx],ALARM_TH_SPACER,tmpstr);
    }
    return tempstring;
}

static void limit_th_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    int idx = (long)data;
    if (Model.telem_alarm_th[idx] < 9) {
        Model.telem_alarm_th[idx]++;
    } else {
        Model.telem_alarm_th[idx] = 0;
    }
}

static void sound_test_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    u8 idx = (long)data;
#if HAS_EXTENDED_AUDIO && HAS_MUSIC_CONFIG
    MUSIC_Play(MUSIC_GetTelemetryAlarm(MUSIC_TELEMALARM1 + idx));
#else
    MUSIC_Play(MUSIC_TELEMALARM1 + idx);
#endif
}

void PAGE_TelemconfigEvent() {
}
