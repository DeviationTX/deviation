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
    return MIXER_SwashType(Model.swash_type);
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
    u8 count = 0;
    u8 pos = 0;
    u8 changed;
    int i, start, end;
    for(i = 0; i < idx; i++) {
        while(proto_strs[pos])
            pos++;
        pos++;
    }
    while(proto_strs[pos+1+count])
        count++;
    start = atoi(proto_strs[pos+1]);
    end = atoi(proto_strs[pos+2]);
    if (count == 2 && (start != 0 || end != 0)) {
        int s1 = 1, s2 = 1;
        if (start < -200 || end > 200) {
            s1 = 10;
            s2 = 50;
        }
        Model.proto_opts[idx] = GUI_TextSelectHelper(Model.proto_opts[idx], start, end, dir, s1, s2, &changed);
        sprintf(tempstring, "%d", Model.proto_opts[idx]);

    } else {
        Model.proto_opts[idx] = GUI_TextSelectHelper(Model.proto_opts[idx], 0, count-1, dir, 1, 1, &changed);
        snprintf(tempstring, sizeof(tempstring), "%s", _tr(proto_strs[pos+Model.proto_opts[idx]+1]));
    }
    if (changed)
        PROTOCOL_SetOptions();  // for devo, it needs to do protocol init as the telemerty state is changed
    return tempstring;
}

/* Functions for the ppm-in capability */
const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 source = *(u8 *)data;
    u8 is_neg = MIXER_SRC_IS_INV(source);
    u8 changed;
    source = GUI_TextSelectHelper(MIXER_SRC(source), 0, NUM_SOURCES, dir, 1, 1, &changed);
    MIXER_SET_SRC_INV(source, is_neg);
    if (changed) {
        *(u8 *)data = source;
    }
    return INPUT_SourceName(tempstring, source);
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
        int newval = GUI_TextSelectHelper(Model.ppm_map[idx], 0, NUM_INPUTS, dir, 1, 1, NULL);
        Model.ppm_map[idx] = INPUT_GetAbbrevSource(Model.ppm_map[idx], newval, dir);
        INPUT_SourceNameAbbrevSwitch(tempstring, Model.ppm_map[idx]);
    }
    return tempstring;
}

const char *set_train_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    int s1, s2, min, max, value;
    u16 *ptr;
    u8 changed;
    if (idx == 0) {
        min = 1;
        max = MAX_PPM_IN_CHANNELS;
        ptr = NULL;
        value = Model.num_ppmin & 0x3f;
        s1 = 1;
        s2 = 1;
    } else {
        s1 = 10;
        s2 = 50;
        if (idx == 1) {
            min = 1000;
            max = 1800;
            ptr = &Model.ppmin_centerpw;
            value = Model.ppmin_centerpw;
        } else {
            min = 100;
            max = 700;
            ptr = &Model.ppmin_deltapw;
            value = Model.ppmin_deltapw;
        }
    }
    value = GUI_TextSelectHelper(value, min, max, dir, s1, s2, &changed);
    if (changed) {
        if(! ptr) {
           Model.num_ppmin = (Model.num_ppmin & 0xc0) | value;
        } else {
           *ptr = value;
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
