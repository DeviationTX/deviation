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

#include "target.h"
#include "pages.h"
#include "config/model.h"
#include "icons.h"

static struct mixer_page * const mp = &pagemem.u.mixer_page;
static u8 show_chantest;
static void templateselect_cb(guiObject_t *obj, void *data);
static void limitselect_cb(guiObject_t *obj, void *data);
static const char *show_source(guiObject_t *obj, void *data);
static u8 scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data);

const char *MIXPAGE_ChannelNameCB(guiObject_t *obj, void *data)
{
    (void)obj;
    return MIXER_SourceName(mp->tmpstr, (long)data + NUM_INPUTS + 1);
}

const char *chan_name_proto_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    char tmp1[5];
    char tmp2[5];
    if ((long)data < PROTO_MAP_LEN && ProtocolChannelMap[Model.protocol]) {
        MIXER_SourceName(tmp1, (long)data + NUM_INPUTS + 1);
        MIXER_SourceName(tmp2, ProtocolChannelMap[Model.protocol][(long)data]);
        sprintf(mp->tmpstr,"%s-%s", tmp1, tmp2);
    } else {
        MIXER_SourceName(mp->tmpstr, (long)data + NUM_INPUTS + 1);
    }
    return mp->tmpstr;
}

static const char *template_name_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    u8 ch = (long)data;
    enum TemplateType template = MIX_GetTemplate(ch);
    return MIXER_TemplateName(template);
}

#define ENTRIES_PER_PAGE (8 > NUM_CHANNELS ? NUM_CHANNELS : 8)
#define MAX_SCROLL (NUM_CHANNELS > ENTRIES_PER_PAGE ? NUM_CHANNELS - ENTRIES_PER_PAGE : NUM_CHANNELS)
void show_page()
{
    int init_y = 40;
    int i;
    if (mp->firstObj) {
        GUI_RemoveHierObjects(mp->firstObj); 
        mp->firstObj = NULL;
    }
    struct Mixer *mix = MIX_GetAllMixers();
    for (i = 0; i < ENTRIES_PER_PAGE; i++) {
        guiObject_t *obj;
        u8 idx;
        int row = init_y + 24 * i;
        u8 ch = mp->top_channel + i;
        obj = GUI_CreateButton(4, row, BUTTON_64x16, chan_name_proto_cb, 0x0000, limitselect_cb, (void *)((long)ch));
        if (! mp->firstObj)
            mp->firstObj = obj;
        for (idx = 0; idx < NUM_MIXERS; idx++)
            if (mix[idx].dest == ch)
                break;
        GUI_CreateButton(132, row, BUTTON_64x16, template_name_cb, 0x0000, templateselect_cb, (void *)((long)ch));
        if (idx != NUM_MIXERS) {
            enum TemplateType template = MIX_GetTemplate(ch);
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

void show_chantest_cb(guiObject_t *obj, void *data)
{
    (void)data;
    (void)obj;
    
    show_chantest = 1;
    PAGE_ChantestModal(PAGE_MixerInit);
}

void PAGE_MixerInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    memset(mp, 0, sizeof(*mp));
    show_chantest = 0;
    mp->firstObj = NULL;
    mp->top_channel = 0;
    PAGE_ShowHeader("Mixer");
    GUI_CreateIcon(224, 0, &icons[ICON_CHANTEST], show_chantest_cb, NULL);
    GUI_CreateScrollbar(304, 32, 208, MAX_SCROLL, NULL, scroll_cb, NULL);
    show_page();
}

static const char *show_source(guiObject_t *obj, void *data)
{
    (void)obj;
    u8 *source = (u8 *)data;
    return MIXER_SourceName(mp->tmpstr, *source);
}

void PAGE_MixerEvent()
{
    if (show_chantest) {
        PAGE_ChantestEvent();
        return;
    }
    if (mp->cur_mixer && mp->graphs[0]) {
        if(MIX_ReadInputs(mp->raw, CHAN_MAX_VALUE / 100)) { // +/-1%
            GUI_Redraw(mp->graphs[0]);
            if (mp->graphs[1])
                GUI_Redraw(mp->graphs[1]);
            if (mp->graphs[2])
                GUI_Redraw(mp->graphs[2]);
        }
    }
}

void templateselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    long idx = (long)data;
    u8 i;

    mp->cur_template = MIX_GetTemplate(idx);
    PAGE_SetModal(1);
    MIX_GetLimit(idx, &mp->limit);
    mp->channel = idx;
    mp->num_complex_mixers = 1;
    for(i = 0; i < sizeof(mp->mixer) / sizeof(struct Mixer); i++)
        MIX_InitMixer(mp->mixer + i, idx);

    if (mp->cur_template != MIXERTEMPLATE_NONE) {
        mp->num_complex_mixers = MIX_GetMixers(idx, mp->mixer, sizeof(mp->mixer) / sizeof(struct Mixer));
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

void limitselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    long ch = (long)data;
    MIX_GetLimit(ch, &mp->limit);
    mp->channel = ch;
    MIXPAGE_EditLimits();
}

static u8 scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data) {
    (void)parent;
    (void)data;
    s16 newpos;
    if (direction > 0) {
        newpos = pos + (direction > 1 ? ENTRIES_PER_PAGE : 1);
        if (newpos > MAX_SCROLL)
            newpos = MAX_SCROLL;
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

