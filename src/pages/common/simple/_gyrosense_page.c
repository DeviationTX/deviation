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

static struct mixer_page * const mp = &pagemem.u.mixer_page;
#define gui (&gui_objs.u.stdgyro)

static GyroOutputChannel gryo_output;
static u8 output[3] ; // use 0-100 instead of -100 to 100 for gyro
static void convert_output_to_percentile();

const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    sprintf (mp->tmpstr, "%s %d", _tr("Value"), idx);
    return mp->tmpstr;
}

static const char *gyro_output_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed = 1;
    gryo_output = GUI_TextSelectHelper(gryo_output, GYROOUTPUT_GEAR , GYROOUTPUT_AUX2, dir, 2, 2, &changed);
    if (changed) {  // switch gyro output between the gear and the aux2
        // Bug fix: properly create mixes for gear and aux2 when gyro channel is changed
        // firstly: dynamically create mixes for target gyro channel
        for (u8 i = 0; i < GYROMIXER_COUNT; i ++) {
            memcpy(&mp->mixer[i],mp->mixer_ptr[i], sizeof(struct Mixer));
            mp->mixer[i].dest = gryo_output;
        }
        MIXER_SetMixers(mp->mixer, GYROMIXER_COUNT);

        // secondly: dynamically create mixes for origin gryo channel
        if (gryo_output == GYROOUTPUT_GEAR) {
            mp->mixer[0].src = INP_MIX0;
            mp->mixer[0].dest = GYROOUTPUT_AUX2;
        } else {
            mp->mixer[0].src = INP_GEAR0;
            mp->mixer[0].dest = GYROOUTPUT_GEAR;
        }
        MIXER_SetMixers(mp->mixer, 1);
        SIMPLEMIX_GetMixers(mp->mixer_ptr, gryo_output, GYROMIXER_COUNT); // must refresh mixer_ptr for next time use
        convert_output_to_percentile();
        for (u8 i = 0; i < GYROMIXER_COUNT; i ++) {
            GUI_Redraw(&gui->gyro[i]);
        }
    }
    if (gryo_output == GYROOUTPUT_GEAR)
        sprintf(mp->tmpstr, "%s/%s5", _tr("Gear"), _tr("Ch"));
    else
        sprintf(mp->tmpstr, "%s/%s7", _tr("Aux2"), _tr("Ch"));
    return mp->tmpstr;
}

static void convert_output_to_percentile()
{
    for (u8 i = 0; i < 3; i++) {
        output[i] = (mp->mixer_ptr[i]->scalar + 100)/2;
    }
}

static const char *gyro_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 i = (long)data;
    u8 changed = 1;
    output[i] = GUI_TextSelectHelper(output[i], 0 , 100, dir, 1, 2, &changed);
    if (changed) {
        mp->mixer_ptr[i]->scalar = output[i] * 2 - 100;
    }
    sprintf(mp->tmpstr, "%d", output[i]);
    strcat(mp->tmpstr, "%");
    return mp->tmpstr;
}


