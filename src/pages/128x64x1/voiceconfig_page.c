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
    LABELID_WIDTH  = LABEL_WIDTH + TEXTSEL_WIDTH,
    MSG_X          = 20,
    MSG_Y          = 10,
};
#endif //OVERRIDE_PLACEMENT

#if HAS_MUSIC_CONFIG

#include "../common/_voiceconfig_page.c"

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;

    //Row 1
    GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, voiceconfig_str_cb, NULL, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->voiceidx[relrow], TEXTSEL_X, y,
            TEXTSEL_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, voice_test_cb, voiceid_cb, (void *)(long)absrow);
    //Row 2
    y += LINE_SPACE;
    GUI_CreateLabelBox(&gui->voicelbl[relrow], LABEL_X, y,
            LABELID_WIDTH, LINE_HEIGHT, &TINY_FONT, voicelbl_cb, NULL, (void *)(long)absrow);
    return 1;
}

void PAGE_VoiceconfigInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    if (!Transmitter.audio_player) {
        GUI_CreateLabelBox(&gui->msg, MSG_X, MSG_Y, 0, 0, &LABEL_FONT, NULL, NULL,
            _tr("External voice \nmust be enabled\nin hardware.ini"));
        return;
    }
    if (PPMin_Mode() || Model.protocol == PROTOCOL_PPM ) {
        GUI_CreateLabelBox(&gui->msg, MSG_X, MSG_Y, 0, 0, &LABEL_FONT, NULL, NULL,
            _tr("External voice not\navailable while\nPPM in use"));
        return;
    }
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_VOICECFG));
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LINE_SPACE * 2, MODEL_CUSTOM_ALARMS, row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);

}
#endif
