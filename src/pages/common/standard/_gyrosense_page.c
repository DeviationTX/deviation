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

static struct gyrosense_page  * const mp  = &pagemem.u.gyrosense_page;
static struct stdgyro_obj * const gui = &gui_objs.u.stdgyro;

static void convert_output_to_percentile();

const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    snprintf (tempstring, sizeof(tempstring), "%s %d", _tr("Value"), idx);
    return tempstring;
}

static const char *gyro_output_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed = 1;
    mp->gyro_output = GUI_TextSelectHelper(mp->gyro_output, GYROOUTPUT_GEAR , GYROOUTPUT_AUX2, dir, 2, 2, &changed);
    if (changed) {  // switch gyro output between the gear and the aux2
        // firstly: dynamically create mixes for target gyro channel
        int i;
        for (i = 0; i < GYROMIXER_COUNT; i ++) {
            if(! mp->mixer_ptr[i])
                break;
            mp->mixer[i] = *mp->mixer_ptr[i];
            mp->mixer[i].dest = mp->gyro_output;
        }
        MIXER_SetTemplate(mp->gyro_output, MIXERTEMPLATE_EXPO_DR);
        // secondly: clear mix for original gyro channel
        MIXER_SetTemplate(mp->gyro_output == GYROOUTPUT_GEAR ? GYROOUTPUT_AUX2 : GYROOUTPUT_GEAR, MIXERTEMPLATE_NONE);
        // save mixers
        MIXER_SetMixers(mp->mixer, i);
        // reload mixers because order may change
        int count = STDMIX_GetMixers(mp->mixer_ptr, mp->gyro_output, GYROMIXER_COUNT);
        convert_output_to_percentile();
        for (i = 0; i < count; i ++) {
            GUI_Redraw(&gui->gyro[i]);
        }
    }
    if (mp->gyro_output == GYROOUTPUT_GEAR)
        snprintf(tempstring, sizeof(tempstring), "%s/%s5", _tr("GEAR"), _tr("Ch"));
    else
        snprintf(tempstring, sizeof(tempstring), "%s/%s7", _tr("AUX2"), _tr("Ch"));
    return tempstring;
}

static void convert_output_to_percentile()
{
    int count = INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_GYROSENSE]);
    for (u8 i = 0; i < count; i++) {
        mp->output[i] = (mp->mixer_ptr[i]->scalar + 100)/2;
    }
}

static const char *gyro_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 i = (long)data;
    u8 changed = 1;
    mp->output[i] = GUI_TextSelectHelper(mp->output[i], 0 , 100, dir, 1, 2, &changed);
    if (changed) {
        mp->mixer_ptr[i]->scalar = mp->output[i] * 2 - 100;
    }
    sprintf(tempstring, "%d%%", mp->output[i]);
    return tempstring;
}


