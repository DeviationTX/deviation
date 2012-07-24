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

static struct mixer_page * const mp = &pagemem.u.mixer_page;

static void templateselect_cb(guiObject_t *obj, void *data);
static void limitselect_cb(guiObject_t *obj, void *data);
static const char *show_source(guiObject_t *obj, void *data);
static u8 scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data);

const char *MIXPAGE_ChannelNameCB(guiObject_t *obj, void *data)
{
    (void)obj;
    return MIXER_SourceName(mp->tmpstr, (long)data + NUM_INPUTS + 1);
}

static const char *template_name_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    u8 ch = (long)data;
    enum TemplateType template = MIX_GetTemplate(ch);
    return MIXPAGE_TemplateName(template);
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
        obj = GUI_CreateButton(8, row, BUTTON_48x16, MIXPAGE_ChannelNameCB, 0x0000, limitselect_cb, (void *)((long)ch));
        if (! mp->firstObj)
            mp->firstObj = obj;
        for (idx = 0; idx < NUM_MIXERS; idx++)
            if (mix[idx].dest == ch)
                break;
        GUI_CreateButton(124, row, BUTTON_64x16, template_name_cb, 0x0000, templateselect_cb, (void *)((long)ch));
        if (idx != NUM_MIXERS) {
            enum TemplateType template = MIX_GetTemplate(ch);
            GUI_CreateLabel(64, row+2, show_source, DEFAULT_FONT, &mix[idx].src);
            if (template == MIXERTEMPLATE_EXPO_DR) {
                if (mix[idx+1].sw) {
                    GUI_CreateLabel(192, row+2, show_source, DEFAULT_FONT, &mix[idx+1].sw);
                }
                if (mix[idx+2].sw) {
                    GUI_CreateLabel(256, row+2, show_source, DEFAULT_FONT, &mix[idx+2].sw);
                }
            }
        }
    }
}

void PAGE_MixerInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    mp->firstObj = NULL;
    mp->top_channel = 0;
    PAGE_ShowHeader("Mixer");
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
    if (mp->cur_mixer && mp->graphs[0]) {
        if(MIX_ReadInputs(mp->raw)) {
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
    mp->link_curves = 0xff;
    for(i = 0; i < sizeof(mp->mixer) / sizeof(struct Mixer); i++)
        MIX_InitMixer(mp->mixer + i, idx);

    if (mp->cur_template != MIXERTEMPLATE_NONE) {
        mp->num_complex_mixers = MIX_GetMixers(idx, mp->mixer, sizeof(mp->mixer) / sizeof(struct Mixer));
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

