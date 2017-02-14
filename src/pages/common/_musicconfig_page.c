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

#include "extended_audio.h"

static struct musicconfig_page * const mp = &pagemem.u.musicconfig_page;
static struct musicconfig_obj * const gui = &gui_objs.u.musicconfig;

static u16 current_selected = 0;

enum {
    MUSIC_SRC_SWITCH = 1,
    MUSIC_SRC_BUTTON,
#if NUM_AUX_KNOBS
    MUSIC_SRC_AUX,
#endif
    MUSIC_SRC_TELEMETRY,
};

static u8 musicconfig_getsrctype (u8 idx)
{
    if (idx < NUM_INPUTS - INP_HAS_CALIBRATION)
        return MUSIC_SRC_SWITCH;
/*    if (idx < NUM_INPUTS - INP_HAS_CALIBRATION + NUM_TX_BUTTONS)
        return MUSIC_SRC_BUTTON;
#if NUM_AUX_KNOBS
    if (idx < NUM_INPUTS - INP_HAS_CALIBRATION + NUM_TX_BUTTONS + NUM_AUX_KNOBS)
        return MUSIC_SRC_AUX;
#endif
*/
    return MUSIC_SRC_TELEMETRY;
}

const char *musicconfig_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    if (musicconfig_getsrctype(idx) == MUSIC_SRC_SWITCH)
        INPUT_SourceName(tempstring,idx + INP_HAS_CALIBRATION + 1);
    if (musicconfig_getsrctype(idx) == MUSIC_SRC_TELEMETRY)
        snprintf(tempstring, sizeof(tempstring), _tr("Telem %d"),
            idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS) + 1);
    return tempstring;
}

static void music_test_cb(guiObject_t *obj, void *data)
{
    (void) obj;
    u8 idx = (long)data;
    if (musicconfig_getsrctype(idx) == MUSIC_SRC_SWITCH) {
        if (Model.music.switches[idx].music)
            MUSIC_Play(Model.music.switches[idx].music);
    }
    if (musicconfig_getsrctype(idx) == MUSIC_SRC_TELEMETRY) {
        if (Model.music.telemetry[idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS)].music)
            MUSIC_Play(Model.music.telemetry[idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS)].music);
    }
}

static const char *musiclbl_cb(guiObject_t *obj, const void *data)
{
    (void) obj;
    int idx = (long)data;
    if (musicconfig_getsrctype(idx) == MUSIC_SRC_SWITCH) {
        if (Model.music.switches[idx].music)
            return music_map[Model.music.switches[idx].music].label;
    }
    /*if (musicconfig_getsrctype(idx) == MUSIC_SRC_BUTTON) {
        if (Model.music.buttons[idx - (NUM_INPUTS - INP_HAS_CALIBRATION)].music)
            return music_map[Model.music.buttons[idx - (NUM_INPUTS - INP_HAS_CALIBRATION)].music].label;
    }*/
    if (musicconfig_getsrctype(idx) == MUSIC_SRC_TELEMETRY) {
        if (Model.music.telemetry[idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS)].music)
            return music_map[Model.music.telemetry[idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS)].music].label;
    }
    return strcpy(tempstring, "");
}

static const char *musicid_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    int cur_row = idx - GUI_ScrollableCurrentRow(&gui->scrollable);
    struct CustomMusic *musicpt;

//    if (musicconfig_getsrctype(idx) == MUSIC_SRC_SWITCH)
        musicpt = &Model.music.switches[idx];
/*    if (musicconfig_getsrctype(idx) == MUSIC_SRC_TELEMETRY)
        musicpt = &Model.music.telemetry[idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS-1)];
*/
    if (dir == -1 && musicpt->music == 0) // set to none below CUSTOM_ALARM_ID
        musicpt->music = 0;
    if (dir == 1 && musicpt->music == 0) // set to CUSTOM_ALARM_ID when currently none
        musicpt->music = CUSTOM_ALARM_ID - 1;

    if (musicpt->music == 0) {
        GUI_Redraw(&gui->musiclbl[cur_row]);
        return strcpy(tempstring, _tr("None"));
    }
    musicpt->music = GUI_TextSelectHelper(musicpt->music, 1, music_map_entries, dir, 1, 10, NULL);
    snprintf(tempstring, 5, "%d", musicpt->music);
    GUI_Redraw(&gui->musiclbl[cur_row]);
    return tempstring;
}
