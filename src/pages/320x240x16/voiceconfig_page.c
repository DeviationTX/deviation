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
#include "gui/gui.h"
#include "music.h"
#include "extended_audio.h"

#if HAS_MUSIC_CONFIG

enum {
    VOICEROWSPACER = (LCD_WIDTH == 480 ? 23 : 20),
    COL1 = 5,
    COL2 = 75,
    COL3 = 140,
};
#include "../common/_voiceconfig_page.c"

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
        int row = y;
        int voice_num = absrow;
        //Row 1
        GUI_CreateLabelBox(&gui->name[relrow], COL1, row, COL2-COL1, 18, &LABEL_FONT,voiceconfig_str_cb, NULL, (void *)(long)voice_num);
        GUI_CreateTextSelect(&gui->voiceidx[relrow],  COL2, row, TEXTSELECT_64, voice_test_cb, voiceid_cb, (void *)(long)voice_num);
        GUI_CreateLabelBox(&gui->voicelbl[relrow], COL3, row, LCD_WIDTH - COL3 - ARROW_WIDTH, 18, &TINY_FONT, voicelbl_cb, NULL, (void *)(long)voice_num);
    return 1;
}

void PAGE_VoiceconfigInit(int page)
{
    (void)page;
    int init_y = 40;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_VOICECFG));

    if ( !AUDIO_VoiceAvailable() ) {
        GUI_CreateLabelBox(&gui->msg, 20, 80, 280, 100, &NARROW_FONT, NULL, NULL,
            _tr("External voice\ncurrently not\navailable"));
        return;
    }
    
    GUI_CreateScrollable(&gui->scrollable, 0, init_y, LCD_WIDTH, LCD_HEIGHT-init_y,
                     VOICEROWSPACER, MODEL_CUSTOM_ALARMS, row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);

}

#endif
