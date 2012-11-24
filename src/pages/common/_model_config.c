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
    sprintf(mp->tmpstr, "%d", (int)Model.swashmix[i]);
    return mp->tmpstr;
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
        Model.proto_opts[idx] = GUI_TextSelectHelper(Model.proto_opts[idx], start, end, dir, 1, 1, &changed);
        sprintf(mp->tmpstr, "%d", Model.proto_opts[idx]);
        return mp->tmpstr;
    } else {
        Model.proto_opts[idx] = GUI_TextSelectHelper(Model.proto_opts[idx], 0, count-1, dir, 1, 1, &changed);
        return _tr(proto_strs[pos+Model.proto_opts[idx]+1]);
    }
}

