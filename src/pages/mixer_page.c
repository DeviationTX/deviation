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

#include "common.h"
#include "pages.h"
#include "config/model.h"
#include "icons.h"

static struct mixer_page * const mp = &pagemem.u.mixer_page;
static u8 show_chantest;
static void templateselect_cb(guiObject_t *obj, const void *data);
static void limitselect_cb(guiObject_t *obj, const void *data);
static const char *show_source(guiObject_t *obj, const void *data);
static u8 scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data);

const char *MIXPAGE_ChannelNameCB(guiObject_t *obj, const void *data)
{
    (void)obj;
    return INPUT_SourceName(mp->tmpstr, (long)data + NUM_INPUTS + 1);
}

const char *MIXPAGE_ChanNameProtoCB(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 ch = (long)data;
    char tmp1[10];

    /* See if we need to name the cyclic virtual channels */
    if (Model.type == MODELTYPE_HELI
        && (ch >= NUM_OUT_CHANNELS && ch < NUM_OUT_CHANNELS + 3))
    {
        int i;
        for (i = 0; i < Model.num_channels; i++) {
            if (Model.template[i] == MIXERTEMPLATE_CYC1
                || Model.template[i] == MIXERTEMPLATE_CYC2
                || Model.template[i] == MIXERTEMPLATE_CYC3)
            {
                switch(ch - NUM_OUT_CHANNELS) {
                    case 0: sprintf(mp->tmpstr, "%s - %s", _tr("CYC"), _tr("AIL")); return mp->tmpstr;
                    case 1: sprintf(mp->tmpstr, "%s - %s", _tr("CYC"), _tr("ELE")); return mp->tmpstr;
                    case 2: sprintf(mp->tmpstr, "%s - %s", _tr("CYC"), _tr("COL")); return mp->tmpstr;
                }
            }
        }
    }
    if (ch < PROTO_MAP_LEN && ProtocolChannelMap[Model.protocol]) {
        INPUT_SourceName(tmp1, ProtocolChannelMap[Model.protocol][ch]);
        sprintf(mp->tmpstr,"%s%d-%s",
            (Model.limits[ch].flags & CH_REVERSE) ? "!" : "",
            (int)(ch + 1), tmp1);
    } else {
        INPUT_SourceName(tmp1, ch + NUM_INPUTS + 1);
        sprintf(mp->tmpstr,"%s%s",
                (ch < Model.num_channels && Model.limits[ch].flags & CH_REVERSE) ? "!" : "",
                tmp1);
    }
    return mp->tmpstr;
}

static const char *template_name_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 ch = (long)data;
    enum TemplateType template = MIXER_GetTemplate(ch);
    return MIXER_TemplateName(template);
}

#define ENTRIES_PER_PAGE (8 > NUM_CHANNELS ? NUM_CHANNELS : 8)
void show_page()
{
    int init_y = 40;
    int i;
    if (mp->firstObj) {
        GUI_RemoveHierObjects(mp->firstObj); 
        mp->firstObj = NULL;
    }
    struct Mixer *mix = MIXER_GetAllMixers();
    for (i = 0; i < ENTRIES_PER_PAGE; i++) {
        guiObject_t *obj;
        u8 idx;
        int row = init_y + 24 * i;
        u8 ch = mp->top_channel + i;
        if (ch >= Model.num_channels)
            ch += (NUM_OUT_CHANNELS - Model.num_channels);
        if (ch < NUM_OUT_CHANNELS)
            obj = GUI_CreateButton(4, row, BUTTON_64x16, MIXPAGE_ChanNameProtoCB,
                                   0x0000, limitselect_cb, (void *)((long)ch));
        else
            obj = GUI_CreateLabelBox(4, row, 64, 16, &DEFAULT_FONT,
                                   MIXPAGE_ChanNameProtoCB, NULL, (void *)((long)ch));
        if (! mp->firstObj)
            mp->firstObj = obj;
        GUI_CreateButton(132, row, BUTTON_64x16, template_name_cb, 0x0000,
                         templateselect_cb, (void *)((long)ch));
        for (idx = 0; idx < NUM_MIXERS; idx++)
            if (mix[idx].dest == ch)
                break;
        if (idx != NUM_MIXERS) {
            enum TemplateType template = MIXER_GetTemplate(ch);
            GUI_CreateLabelBox(68, row, 60, 16, &NARROW_FONT, show_source, NULL, &mix[idx].src);
            if (template == MIXERTEMPLATE_EXPO_DR) {
                if (mix[idx].src == mix[idx+1].src && mix[idx].dest == mix[idx+1].dest && mix[idx+1].sw) {
                    GUI_CreateLabelBox(200, row, 52, 16, &SMALL_FONT, show_source, NULL, &mix[idx+1].sw);
                }
                if (mix[idx].src == mix[idx+2].src && mix[idx].dest == mix[idx+2].dest && mix[idx+2].sw) {
                    GUI_CreateLabelBox(252, row, 52, 16, &SMALL_FONT, show_source, NULL, &mix[idx+2].sw);
                }
            }
        }
    }
}

void show_chantest_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    
    show_chantest = 1;
    PAGE_ChantestModal(PAGE_MixerInit);
}

void PAGE_MixerInit(int page)
{
    PAGE_SetModal(0);
    memset(mp, 0, sizeof(*mp));
    mp->top_channel = page;
    mp->max_scroll = Model.num_channels + NUM_VIRT_CHANNELS > ENTRIES_PER_PAGE ? Model.num_channels + NUM_VIRT_CHANNELS - ENTRIES_PER_PAGE : Model.num_channels + NUM_VIRT_CHANNELS;
    show_chantest = 0;
    mp->firstObj = NULL;
    PAGE_ShowHeader("Mixer");
    GUI_CreateIcon(224, 0, &icons[ICON_CHANTEST], show_chantest_cb, NULL);
    GUI_CreateScrollbar(304, 32, 208, mp->max_scroll, NULL, scroll_cb, NULL);
    show_page();
}

static const char *show_source(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 *source = (u8 *)data;
    return INPUT_SourceName(mp->tmpstr, *source);
}

void PAGE_MixerEvent()
{
    if (show_chantest) {
        PAGE_ChantestEvent();
        return;
    }
    if (mp->cur_mixer && mp->graphs[0]) {
        if(MIXER_ReadInputs(mp->raw, CHAN_MAX_VALUE / 100)) { // +/-1%
            GUI_Redraw(mp->graphs[0]);
            if (mp->graphs[1])
                GUI_Redraw(mp->graphs[1]);
            if (mp->graphs[2])
                GUI_Redraw(mp->graphs[2]);
        }
    }
}

void templateselect_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long idx = (long)data;
    u8 i;
    mp->cur_template = MIXER_GetTemplate(idx);
    PAGE_SetModal(1);
    MIXER_GetLimit(idx, &mp->limit);
    mp->channel = idx;
    mp->num_complex_mixers = 1;
    for(i = 0; i < sizeof(mp->mixer) / sizeof(struct Mixer); i++)
        MIXER_InitMixer(mp->mixer + i, idx);

    if (mp->cur_template != MIXERTEMPLATE_NONE) {
        mp->num_complex_mixers = MIXER_GetMixers(idx, mp->mixer, sizeof(mp->mixer) / sizeof(struct Mixer));
    }
    mp->link_curves = 0xff;
    if (mp->cur_template == MIXERTEMPLATE_EXPO_DR) {
        if (mp->num_complex_mixers > 1) {
            if (memcmp(&mp->mixer[1].curve, &mp->mixer[0].curve, sizeof(struct Curve)) != 0) {
                mp->link_curves &= ~0x01;
            }
            if (mp->num_complex_mixers > 2) {
                if (memcmp(&mp->mixer[2].curve, &mp->mixer[0].curve, sizeof(struct Curve)) != 0) {
                    mp->link_curves &= ~0x02;
                }
            }
        }
    }
    MIXPAGE_ChangeTemplate(1);
}

void limitselect_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long ch = (long)data;
    MIXER_GetLimit(ch, &mp->limit);
    mp->channel = ch;
    MIXPAGE_EditLimits();
}

static u8 scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data) {
    (void)parent;
    (void)data;
    s16 newpos;
    if (direction > 0) {
        newpos = pos + (direction > 1 ? ENTRIES_PER_PAGE : 1);
        if (newpos > mp->max_scroll)
            newpos = mp->max_scroll;
    } else {
        newpos = pos - (direction < -1 ? ENTRIES_PER_PAGE : 1);
        if (newpos < 0)
            newpos = 0;
    }
    if (newpos != pos) {
        mp->top_channel = newpos;
        show_page();
    }
    return newpos;
}

