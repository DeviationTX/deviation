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

#if HAS_MUSIC_CONFIG

enum {
    MUSICROWSPACER = (LCD_WIDTH == 480 ? 23 : 20),
    COL1 = 5,
    COL2 = 75,
    COL3 = 140,
};
#include "../common/_musicconfig_page.c"

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
        int row = y;
        int music_num = absrow;
        //Row 1
        GUI_CreateLabelBox(&gui->name[relrow], COL1, row, COL2-COL1, 18, &LABEL_FONT,musicconfig_str_cb, NULL, (void *)(long)music_num);
        GUI_CreateTextSelect(&gui->musicidx[relrow],  COL2, row, TEXTSELECT_64, music_test_cb, musicid_cb, (void *)(long)music_num);
        GUI_CreateLabelBox(&gui->musiclbl[relrow], COL3, row, LCD_WIDTH - COL3 - ARROW_WIDTH, 18, &TINY_FONT, musiclbl_cb, NULL, (void *)(long)music_num);
    return 1;
}

void PAGE_MusicconfigInit(int page)
{
    (void)page;
    int init_y = 40;
    PAGE_SetModal(0);
    if (Transmitter.audio_player == 0) {
        GUI_CreateLabelBox(&gui->msg, 20, 80, 280, 100, &NARROW_FONT, NULL, NULL,
             _tr("External audio support\nmust be enabled\nin hardware.ini"));
        return;
    }
    if (PPMin_Mode() || Model.protocol == PROTOCOL_PPM ) {
        GUI_CreateLabelBox(&gui->msg, 20, 80, 280, 100, &NARROW_FONT, NULL, NULL,
            _tr("External audio not\navailable while\nPPM in use"));
        return;
    }
    PAGE_ShowHeader(PAGE_GetName(PAGEID_MUSICCFG));

    GUI_CreateScrollable(&gui->scrollable, 0, init_y, LCD_WIDTH, LCD_HEIGHT-init_y,
                     MUSICROWSPACER, MODEL_CUSTOM_ALARMS, row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);

}

#endif
