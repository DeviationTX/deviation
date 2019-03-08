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
#include <stdlib.h>

static struct model_page * const mp = &pagemem.u.model_page;
static const char **proto_strs;

static const char *swash_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    Model.swash_type = GUI_TextSelectHelper(Model.swash_type, 0 , SWASH_TYPE_90, dir, 1, 1, NULL);
    return _tr(MIXER_SwashType(Model.swash_type));
}

static const char *swashinv_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 mask = (long)data;
    u8 val = Model.swash_invert & mask ? 1 : 0;
    val = GUI_TextSelectHelper(val, 0 , 1, dir, 1, 1, NULL);
    Model.swash_invert = val ? Model.swash_invert | mask : Model.swash_invert & ~mask;
    switch(val) {
        case 0: return _tr("Normal");
        case 1: return _tr("Inverted");
    }
    return "";
}
void swashinv_press_cb(guiObject_t *obj, void *data)
{
    u8 mask = (long)data;
    Model.swash_invert ^= mask;
    GUI_Redraw(obj);
}

static const char *swashmix_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    int i = (long)data;
    Model.swashmix[i] = GUI_TextSelectHelper(Model.swashmix[i], 0, 100, dir, 1, 5, NULL);
    snprintf(tempstring, sizeof(tempstring), "%d", (int)Model.swashmix[i]);
    return tempstring;
}

static const char *proto_opt_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    long idx = (long)data;
    unsigned count = 0;
    unsigned pos = 0;
    unsigned step = 0;
    u8 changed;
    int i, start, end;
    for(i = 0; i < idx; i++) {
        while(proto_strs[pos])
            pos++;
        pos++;
    }
    while(proto_strs[pos+1+count])
        count++;
    start = exact_atoi(proto_strs[pos+1]);
    end   = exact_atoi(proto_strs[pos+2]);
    if(count == 3)
        step = exact_atoi(proto_strs[pos+3]);
    if ((start != 0 || end != 0) && (count == 2 || (count == 3 && step != 0))) {
        int s1 = 1, s2 = 1;
        if (step) {
            s1 = step & 0xffff;
            s2 = (step >> 16) & 0xffff;
            if(s2 == 0)
                s2 = s1;
            if(s1 == 0)
                s1 = 1;
        }
        Model.proto_opts[idx] = GUI_TextSelectHelper(Model.proto_opts[idx], start, end, dir, s1, s2, &changed);
        sprintf(tempstring, "%d", Model.proto_opts[idx]);

    } else {
        Model.proto_opts[idx] = GUI_TextSelectHelper(Model.proto_opts[idx], 0, count-1, dir, 1, 1, &changed);
        tempstring_cpy(_tr(proto_strs[pos+Model.proto_opts[idx]+1]));
    }
    if (changed)
        PROTOCOL_SetOptions();  // for devo, it needs to do protocol init as the telemerty state is changed
    return tempstring;
}

/* Functions for the ppm-in capability */
const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int source = INPUT_SelectSource(*(u8 *)data, dir, NULL);
    *(u8 *)data = source;
    return INPUT_SourceName(tempstring, source);
}

const char *set_input_source_cb(guiObject_t *obj, int src, int value, void *data) {
    (void)obj;
    (void)value;
    *(u8 *)data = INPUT_SelectInput(*(u8 *)data, src, NULL);
    return INPUT_SourceName(tempstring, *(u8 *)data);
}

void sourceselect_cb(guiObject_t *obj, void *data)
{
    u8 *source = (u8 *)data;
    MIXER_SET_SRC_INV(*source, ! MIXER_SRC_IS_INV(*source));
    GUI_Redraw(obj);
}

const char *set_chmap_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    if (PPMin_Mode() == PPM_IN_TRAIN1) {
        if (Model.ppm_map[idx] >= Model.num_channels)
            Model.ppm_map[idx] = -1;
        Model.ppm_map[idx] = GUI_TextSelectHelper(Model.ppm_map[idx], -1, Model.num_channels-1, dir, 1, 1, NULL);
        if (Model.ppm_map[idx] < 0)
            return _tr("None");
        snprintf(tempstring, sizeof(tempstring), _tr("Ch%d"), Model.ppm_map[idx]+1);
    } else {
        Model.ppm_map[idx] = INPUT_SelectAbbrevSource(Model.ppm_map[idx], dir);
        INPUT_SourceNameAbbrevSwitch(tempstring, Model.ppm_map[idx]);
    }
    return tempstring;
}

const char *set_train_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    int s1, s2, min, max, value;
    u8 changed;
    if (idx == 0) {
        min = 1;
        max = MAX_PPM_IN_CHANNELS;
        value = Model.num_ppmin_channels;
        s1 = 1;
        s2 = 1;
    } else {
        s1 = 10;
        s2 = 50;
        if (idx == 1) {
            min = 1000;
            max = 1800;
            value = Model.ppmin_centerpw;
        } else {
            min = 100;
            max = 700;
            value = Model.ppmin_deltapw;
        }
    }
    value = GUI_TextSelectHelper(value, min, max, dir, s1, s2, &changed);
    if (changed) {
        switch (idx)
        {
            case 0:
                Model.num_ppmin_channels = value;
                break;
            case 1:
                Model.ppmin_centerpw = value;
                break;
            case 2:
                Model.ppmin_deltapw = value;
                break;
        }
    }
    sprintf(tempstring, "%d", value);
    return tempstring;
}

const char *input_chname_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    snprintf(tempstring, sizeof(tempstring), _tr("PPM%d"), idx+1);
    return tempstring;
}

#if HAS_VIDEO
const char *set_videoch_cb(guiObject_t *obj, int dir, const void *data)
{
    (void)obj;
    (void)data;
    int value = Model.videoch;
    u8 changed;
    value = GUI_TextSelectHelper(value, 0, HAS_VIDEO-1, dir, 1, 1, &changed);
    if (changed) {
        VIDEO_SetChannel(value);
        Model.videoch = value;
    }
    sprintf(tempstring, "%d", value);
    return tempstring;
}
const char *set_videocontrast_cb(guiObject_t *obj, int dir, const void *data)
{
    (void)obj;
    (void)data;
    int value = Model.video_contrast;
    u8 changed;
    value = GUI_TextSelectHelper(value, -10, 10, dir, 1, 1, &changed);
    if (changed) {
        VIDEO_Contrast(value);
        Model.video_contrast = value;
    }
    sprintf(tempstring, "%d", value);
    return tempstring;
}
const char *set_videobrightness_cb(guiObject_t *obj, int dir, const void *data)
{
    (void)obj;
    (void)data;
    int value = Model.video_brightness;
    u8 changed;
    value = GUI_TextSelectHelper(value, -10, 10, dir, 1, 1, &changed);
    if (changed) {
        VIDEO_Brightness(value);
        Model.video_brightness = value;
    }
    sprintf(tempstring, "%d", value);
    return tempstring;
}
#endif //HAS_VIDEO
