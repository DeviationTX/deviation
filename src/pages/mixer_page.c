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

const char *channel_name[] = {
    "Ch1", "Ch2", "Ch3", "Ch4",
    "Ch5", "Ch6", "Ch7", "Ch8",
    "Ch9", "Ch10", "Ch11", "Ch12",
    "Ch13", "Ch14", "Ch15", "Ch16"
};

static void templateselect_cb(guiObject_t *obj, void *data);
static void limitselect_cb(guiObject_t *obj, void *data);
static const char *show_source(guiObject_t *obj, void *data);

#define ENTRIES_PER_PAGE 9

void PAGE_MixerInit(int page)
{
    int init_y = 8;
    int i;
    mp->modifying_template = 0;
    struct Mixer *mix = MIX_GetAllMixers();
    for (i = 0; i < ENTRIES_PER_PAGE; i++) {
        u8 idx;
        int row = init_y + 26 * i;
        u8 ch = ENTRIES_PER_PAGE * page + i;
        enum TemplateType template = MIX_GetTemplate(ch);
        GUI_CreateButton(10, row, BUTTON_48x16, channel_name[ENTRIES_PER_PAGE * page + i], 0x0000, limitselect_cb, (void *)((long)ch));
        for (idx = 0; idx < NUM_MIXERS; idx++)
            if (mix[idx].dest == ch)
                break;
        GUI_CreateButton(124, row, BUTTON_64x16, MIXPAGE_TemplateName(template), 0x0000, templateselect_cb, (void *)((long)ch));
        if (idx != NUM_MIXERS) {
            GUI_CreateLabel(60, row+2, show_source, DEFAULT_FONT, &mix[idx].src);
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

static const char *show_source(guiObject_t *obj, void *data)
{
    (void)obj;
    u8 *source = (u8 *)data;
    return MIXPAGE_SourceName(*source);
}

void PAGE_MixerEvent()
{
    if(mp->graph && mp->cur_mixer) {
        if(MIX_ReadInputs(mp->raw))
            GUI_Redraw(mp->graph);
    }
}

int PAGE_MixerCanChange()
{
    return ! mp->modifying_template;
}

void templateselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    long idx = (long)data;
    u8 i;

    mp->cur_template = MIX_GetTemplate(idx);
    mp->modifying_template = 1;
    MIX_GetLimit(idx, &mp->limit);
    mp->channel = idx;
    mp->num_complex_mixers = 1;
    mp->link_curves = 0xff;
    for(i = 0; i < sizeof(mp->mixer) / sizeof(struct Mixer); i++)
        MIX_InitMixer(mp->mixer + i, idx);

    if (mp->cur_template != MIXERTEMPLATE_NONE) {
        mp->num_complex_mixers = MIX_GetMixers(idx, mp->mixer, sizeof(mp->mixer) / sizeof(struct Mixer));
    }
    MIXPAGE_ChangeTemplate();
}

void limitselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    long ch = (long)data;
    MIX_GetLimit(ch, &mp->limit);
    mp->channel = ch;
    MIXPAGE_EditLimits();
}
