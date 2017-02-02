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
    MUSICROWS = 4,
    MUSICROWSPACER = (LCD_WIDTH == 480 ? 45 : 40),
    COL1 = 5,
    COL2 = 70,
    COL3 = 175,
    COL4 = 225,
};
#include "../common/_musicconfig_page.c"

/*            Advanced
   Row1       Source
   Row2       Music-nr
   Row3       Music-label
   Row4       Volume
*/

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
        int row = y;
        int music_num = absrow;
        //Row 1
        GUI_CreateLabelBox(&gui->name[relrow], COL1, row, COL2-COL1, 18, &DEFAULT_FONT,musicconfig_str_cb, NULL, (void *)(long)music_num);
        GUI_CreateTextSelect(&gui->musicsrc[relrow], COL2, row, TEXTSELECT_96, toggle_musicsrc_cb, musicsrc_cb, (void *)(long)music_num);
        GUI_CreateLabelBox(&gui->idxlbl[relrow], COL3, row, COL4-COL3, 18, &DEFAULT_FONT, NULL, NULL,  _tr("MP3 ID"));
        GUI_CreateTextSelect(&gui->musicidx[relrow],  COL4, row, TEXTSELECT_64, music_test_cb, musicid_cb, (void *)(long)music_num);
        //Row 2
        row+=20;
        GUI_CreateLabelBox(&gui->musiclbl[relrow], COL1, row, LCD_WIDTH-25, 18, &DEFAULT_FONT, musiclbl_cb, NULL, (void *)(long)music_num);
    return 2;
}

void PAGE_MusicconfigInit(int page)
{
    (void)page;
    int init_y = 40;
    PAGE_SetModal(0);
    if (Transmitter.audio_player == 0) {
        GUI_CreateLabelBox(&gui->msg, 20, 80, 280, 100, &NARROW_FONT, NULL, NULL, _tr("MP3 player support must\nbe enabled in hardware.ini"));
        return;
    }
    PAGE_ShowHeader(PAGE_GetName(PAGEID_MUSICCFG));

    GUI_CreateScrollable(&gui->scrollable, 0, init_y, LCD_WIDTH, LCD_HEIGHT-init_y,
                     MUSICROWSPACER, MODEL_CUSTOM_ALARMS, row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);

}

#endif
