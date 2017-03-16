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

static struct voiceconfig_page * const mp = &pagemem.u.voiceconfig_page;
static struct voiceconfig_obj * const gui = &gui_objs.u.voiceconfig;

static u16 current_selected = 0;

enum {
    VOICE_SRC_SWITCH = 1,
    VOICE_SRC_BUTTON,
#if NUM_AUX_KNOBS
    VOICE_SRC_AUX,
#endif
    VOICE_SRC_TIMER,
    VOICE_SRC_TELEMETRY,
    VOICE_SRC_MIXER,
};

static u8 voiceconfig_getsrctype (u8 idx)
{
    if (idx < NUM_SWITCHES)
        return VOICE_SRC_SWITCH;
/*    if (idx < NUM_INPUTS - INP_HAS_CALIBRATION + NUM_TX_BUTTONS)
        return VOICE_SRC_BUTTON;*/
#if NUM_AUX_KNOBS
    if (idx < NUM_SWITCHES + NUM_AUX_KNOBS * 2)
        return VOICE_SRC_AUX;
#endif
    if (idx < NUM_SWITCHES + NUM_AUX_KNOBS * 2 + NUM_TIMERS)
        return VOICE_SRC_TIMER;
    if (idx < NUM_SWITCHES + NUM_AUX_KNOBS * 2 + NUM_TIMERS + TELEM_NUM_ALARMS)
        return VOICE_SRC_TELEMETRY;
    return VOICE_SRC_MIXER;
}

const char *voiceconfig_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_SWITCH)
        INPUT_SourceName(tempstring,idx + INP_HAS_CALIBRATION + 1);
#if NUM_AUX_KNOBS
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_AUX) {
        if ((idx - NUM_SWITCHES) % 2) {
            INPUT_SourceName(tempstring,(idx - NUM_SWITCHES -1) / 2 + NUM_STICKS + 1);
            strcat(tempstring, " ");
            strcat(tempstring, _tr("up"));
        }
        else {
            INPUT_SourceName(tempstring,(idx - NUM_SWITCHES) / 2 + NUM_STICKS + 1);
            strcat(tempstring," ");
            strcat(tempstring, _tr("down"));
        }
    }
#endif
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_TIMER)
        snprintf(tempstring, sizeof(tempstring), _tr("Timer %d"),
            idx - (MODEL_CUSTOM_ALARMS - NUM_TIMERS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS) + 1);
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_TELEMETRY)
        snprintf(tempstring, sizeof(tempstring), _tr("Telem %d"),
            idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS) + 1);
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_MIXER)
        INPUT_SourceNameReal(tempstring,
                (idx - (MODEL_CUSTOM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS) + NUM_INPUTS + 1));

    return tempstring;
}

static void voice_test_cb(guiObject_t *obj, void *data)
{
    (void) obj;
    u8 idx = (long)data;
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_SWITCH) {
        if (Model.voice.switches[idx].music)
            MUSIC_Play(Model.voice.switches[idx].music);
    }
#if NUM_AUX_KNOBS
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_AUX)
        if (Model.voice.aux[idx - NUM_SWITCHES].music)
            MUSIC_Play(Model.voice.aux[idx - NUM_SWITCHES].music);
#endif
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_TIMER) {
        if (Model.voice.timer[idx - (MODEL_CUSTOM_ALARMS - NUM_TIMERS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music)
            MUSIC_Play(Model.voice.timer[idx - (MODEL_CUSTOM_ALARMS - NUM_TIMERS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music);
    }
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_TELEMETRY) {
        if (Model.voice.telemetry[idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music)
            MUSIC_Play(Model.voice.telemetry[idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music);
    }
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_MIXER) {
        if (Model.voice.mixer[idx - (MODEL_CUSTOM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music)
            MUSIC_Play(Model.voice.mixer[idx - (MODEL_CUSTOM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music);
    }
}

static const char *voicelbl_cb(guiObject_t *obj, const void *data)
{
    (void) obj;
    int idx = (long)data;
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_SWITCH) {
        if (Model.voice.switches[idx].music)
            return voice_map[Model.voice.switches[idx].music].label;
    }
#if NUM_AUX_KNOBS
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_AUX) {
        if(Model.voice.aux[idx - NUM_SWITCHES].music)
            return voice_map[Model.voice.aux[idx - NUM_SWITCHES].music].label;
    }
#endif
    /*if (voiceconfig_getsrctype(idx) == VOICE_SRC_BUTTON) {
        if (Model.voice.buttons[idx - (NUM_INPUTS - INP_HAS_CALIBRATION)].music)
            return voice_map[Model.voice.buttons[idx - (NUM_INPUTS - INP_HAS_CALIBRATION)].music].label;
    }*/
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_TIMER) {
        if (Model.voice.timer[idx - (MODEL_CUSTOM_ALARMS - NUM_TIMERS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music)
            return voice_map[Model.voice.timer[idx - (MODEL_CUSTOM_ALARMS - NUM_TIMERS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music].label;
    }
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_TELEMETRY) {
        if (Model.voice.telemetry[idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music)
            return voice_map[Model.voice.telemetry[idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music].label;
    }
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_MIXER) {
        if (Model.voice.mixer[idx - (MODEL_CUSTOM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music)
            return voice_map[Model.voice.mixer[idx - (MODEL_CUSTOM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)].music].label;
    }
    return strcpy(tempstring, "");
}

static const char *voiceid_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    int idx = (long)data;
    int cur_row = idx - GUI_ScrollableCurrentRow(&gui->scrollable);
    struct CustomVoice *vpt;

    if (voiceconfig_getsrctype(idx) == VOICE_SRC_SWITCH)
        vpt = &Model.voice.switches[idx];
#if NUM_AUX_KNOBS
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_AUX)
        vpt = &Model.voice.aux[idx - NUM_SWITCHES];
#endif
    if (voiceconfig_getsrctype(idx) == VOICE_SRC_TIMER)
        vpt = &Model.voice.timer[idx - (MODEL_CUSTOM_ALARMS - NUM_TIMERS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)];

    if (voiceconfig_getsrctype(idx) == VOICE_SRC_TELEMETRY)
        vpt = &Model.voice.telemetry[idx - (MODEL_CUSTOM_ALARMS - TELEM_NUM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)];

    if (voiceconfig_getsrctype(idx) == VOICE_SRC_MIXER)
        vpt = &Model.voice.mixer[idx - (MODEL_CUSTOM_ALARMS - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS)];

    if (dir == -1 && vpt->music == CUSTOM_ALARM_ID) // set to none below 1
        vpt->music = 0;
    if (dir == 1 && vpt->music == 0) // set to CUSTOM_ALARM_ID when currently none
        vpt->music = CUSTOM_ALARM_ID - 1;
    GUI_TextSelectEnablePress((guiTextSelect_t *)obj, vpt->music);

    if (vpt->music == 0) {
        GUI_Redraw(&gui->voicelbl[cur_row]);
        return strcpy(tempstring, _tr("None"));
    }
    vpt->music = GUI_TextSelectHelper(vpt->music - CUSTOM_ALARM_ID + 1, //Relabling so voice in menu starts with 1
        1, voice_map_entries - CUSTOM_ALARM_ID, dir, 1, 10, NULL) + CUSTOM_ALARM_ID - 1;
    snprintf(tempstring, 5, "%d", vpt->music - CUSTOM_ALARM_ID + 1);
    GUI_Redraw(&gui->voicelbl[cur_row]);
    return tempstring;
}
