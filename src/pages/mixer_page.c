/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Foobar is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

static const char *inp[] = {
    "THR", "RUD", "ELE", "AIL",
    "", "D/R", "D/R-C1", "D/R-C2"
};


void PAGE_MixerInit(int page)
{
    int init_y = 16;
    int i;
    mp->modifying_template = 0;
    for (i = 0; i < ENTRIES_PER_PAGE; i++) {
        void *ptr = (void *)((long)i);
        int row = init_y + 28 * i;
        GUI_CreateLabel(10, row, channel_name[ENTRIES_PER_PAGE * page + i], 0x0000);
        strcpy(mp->input_str[i], (i < 4) ? inp[i] : "");
        GUI_CreateLabel(40, row, mp->input_str[i], 0x0000);
        GUI_CreateButton(100, row - 4, BUTTON_90, MIXPAGE_TemplateName(MIX_GetTemplate(i)), 0x0000, templateselect_cb, ptr);
        strcpy(mp->switch_str[i], (i < 4) ? inp[i+4] : "");
        GUI_CreateLabel(240, row, mp->switch_str[i], 0x0000);
    }
    GUI_DrawScreen();
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
    mp->num_complex_mixers = 0;
    mp->link_curves = 0xff;
    for(i = 0; i < sizeof(mp->mixer) / sizeof(struct Mixer); i++)
        MIX_InitMixer(mp->mixer + i, idx);

    if (mp->cur_template != MIXERTEMPLATE_NONE) {
        mp->num_complex_mixers = MIX_GetMixers(idx, mp->mixer, sizeof(mp->mixer) / sizeof(struct Mixer));
    }
    MIXPAGE_ChangeTemplate();
}
