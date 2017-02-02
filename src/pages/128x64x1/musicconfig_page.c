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

#ifndef OVERRIDE_PLACEMENT
enum {
    LABEL_X        = 0,
    LABEL_WIDTH    = 55,
    TEXTSEL_X      = 55,
    TEXTSEL_WIDTH  = 65,
    RESET_X        = 58,
    RESET_WIDTH    = 59,
    START_WIDTH    = 50,
    MSG_X          = 20,
    MSG_Y          = 10,
};
#endif //OVERRIDE_PLACEMENT

#if HAS_MUSIC_CONFIG

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
    (void)relrow;
    u8 space = LINE_SPACE;
    //Row 1
    GUI_CreateLabelBox(&gui->name, LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, musicconfig_str_cb, NULL, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->musicsrc, TEXTSEL_X, y,
            TEXTSEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, toggle_musicsrc_cb, musicsrc_cb, (void *)(long)absrow);
    //Row 2
    y += space;
    GUI_CreateLabelBox(&gui->idxlbl, LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("MP3 ID"));
    GUI_CreateTextSelectPlate(&gui->musicidx, TEXTSEL_X, y,
            TEXTSEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, music_test_cb, musicid_cb, (void *)(long)absrow);
    y += space;
    //Row 3
    GUI_CreateLabelBox(&gui->musiclbl, LABEL_X, y,
            LABEL_WIDTH + TEXTSEL_WIDTH, LINE_HEIGHT, &TINY_FONT, musiclbl_cb, NULL, (void *)(long)absrow);
    y += space;
    //Row 4
/*    GUI_CreateLabelBox(&gui->vollbl, LABEL_X, y ,
            LABEL_WIDTH, LINE_HEIGHT,&DEFAULT_FONT, NULL, NULL, _tr("Volume"));
    GUI_CreateTextSelectPlate(&gui->setvol, TEXTSEL_X, y,
            TEXTSEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, music_test_cb, setvol_cb, (void *)(long)absrow);
*/
    return 1;
}

void PAGE_MusicconfigInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    if (Transmitter.audio_player == 0) {
        GUI_CreateLabelBox(&gui->msg, MSG_X, MSG_Y, 0, 0, &DEFAULT_FONT, NULL, NULL,
            _tr("MP3 player support must\nbe enabled in hardware.ini"));
        return;
    }
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_MUSICCFG));
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LCD_HEIGHT - HEADER_HEIGHT, MODEL_CUSTOM_ALARMS, row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);

}
#endif
