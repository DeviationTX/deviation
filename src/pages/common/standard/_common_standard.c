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

#include "common_standard.h"

static struct mixer_page * const mp = &pagemem.u.mixer_page;

const char *STDMIX_channelname_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 ch = (long)data;
    if (ch < PROTO_MAP_LEN && ProtocolChannelMap[Model.protocol]) {
        char tmp1[30];
        INPUT_SourceNameAbbrevSwitch(tmp1, ProtocolChannelMap[Model.protocol][ch]);
        snprintf(tempstring, sizeof(tempstring), "%d-%s", ch+1, tmp1);
    }
    else if (ch == 5)
        snprintf(tempstring, sizeof(tempstring), "%d-%s",  ch+1, _tr("PIT"));  // aux1
    else
        sprintf(tempstring, "%d-%s%d",ch+1, "AUX", ch -4); //AUX no need to translate
    return tempstring;
}

int STDMIX_GetMixers(struct Mixer **mixers, u8 dest_channel, int count)
{
    u8 idx;
    u8 i = 0;
    struct Mixer *mix = MIXER_GetAllMixers();
    for (idx = 0; idx < NUM_MIXERS; idx++) {
        if (i >= count)
            break;
        if (mix[idx].src!= 0 && mix[idx].dest == dest_channel) {
            mixers[i++] = &mix[idx];
        }
    }
    while(i < count)
        mixers[--count] = 0;
    return i;
}

const char *STDMIX_ModeName(PitThroMode pit_mode)
{
    switch (pit_mode) {
    case PITTHROMODE_NORMAL:
        return _tr("Normal");
    case PITTHROMODE_IDLE1:
        return _tr("Idle Up 1");
        break;
    case PITTHROMODE_IDLE2:
        return _tr("Idle Up 2");
    default: // PITTHROMODE_HOLD
        return _tr("Thr hold");
    }
}

s32 STDMIX_EvalMixerCb(s32 xval, struct Mixer *mix, s32 max_value, s32 min_value)
{
    if (MIXER_SRC_IS_INV(mix->src))
        xval = -xval;
    s32 yval = CURVE_Evaluate(xval, &mix->curve);
    yval = yval * mix->scalar / 100 + PCT_TO_RANGE(mix->offset);

    if (yval > max_value)
        yval = max_value;
    else if (yval <min_value)
        yval = min_value;
    return yval;
}

const char *STDMIX_TitleString(guiObject_t *obj, const void *data)
{
    (void)obj;
    int pageid = ((unsigned long)data) & 0xFFFF;
    int sw     = (((unsigned long)data) >> 16) & 0xFFFF;
    sprintf(tempstring, "%s - ", PAGE_GetName(pageid));
    INPUT_SourceNameAbbrevSwitch(tempstring+strlen(tempstring), mapped_std_channels.switches[sw]);
    return tempstring;
}
