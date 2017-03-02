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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "../pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "mixer.h"
#include "standard.h"
#include "mixer_standard.h"

enum {
    HEADER_OFFSET  = 1,
    MESSAGE_Y      = 10,
    FIELD_X        = 63,
    FIELD_WIDTH1   = 55,
    FIELD_WIDTH2   = 40,
    LABEL_X        = 0,
    LABEL_WIDTH    = FIELD_X - LABEL_X,
};
#endif //OVERRIDE_PLACEMENT

#if HAS_STANDARD_GUI
#include "../../common/standard/_gyrosense_page.c"

void PAGE_GyroSenseInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    memset(mp, 0, sizeof(*mp));
    int expected = INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_GYROSENSE]);
    int count = STDMIX_GetMixers(mp->mixer_ptr, mapped_std_channels.aux2, GYROMIXER_COUNT);
    if (! count) {
        count = STDMIX_GetMixers(mp->mixer_ptr, mapped_std_channels.gear, GYROMIXER_COUNT);
    }
    if (count != expected) {
        GUI_CreateLabelBox(&gui->msg, 0, MESSAGE_Y, 0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }
    gyro_output = mp->mixer_ptr[0]->dest;
    convert_output_to_percentile();
    PAGE_ShowHeader(_tr("Gyro sense"));

    u8 y = HEADER_HEIGHT + HEADER_OFFSET;
    GUI_CreateLabelBox(&gui->chanlbl, LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, NULL, NULL, _tr("Channel"));
    GUI_CreateTextSelectPlate(&gui->chan, FIELD_X, y, FIELD_WIDTH1, LINE_HEIGHT, &TEXTSEL_FONT, NULL, gyro_output_cb, NULL);

    y += LINE_SPACE;
    GUI_CreateLabelBox(&gui->gyrolbl[0], LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, label_cb, NULL, (void *)(long)0);
    GUI_CreateTextSelectPlate(&gui->gyro[0], FIELD_X, y, FIELD_WIDTH2, LINE_HEIGHT, &TEXTSEL_FONT, NULL, gyro_val_cb, (void *)0);

    y += LINE_SPACE;
    GUI_CreateLabelBox(&gui->gyrolbl[1], LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, label_cb, NULL, (void *)(long)1);
    GUI_CreateTextSelectPlate(&gui->gyro[1], FIELD_X, y, FIELD_WIDTH2, LINE_HEIGHT, &TEXTSEL_FONT, NULL, gyro_val_cb, (void *)1);

    if(INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_GYROSENSE]) == 3) {
        y += LINE_SPACE;
        GUI_CreateLabelBox(&gui->gyrolbl[2], LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, label_cb, NULL, (void *)(long)2);
        GUI_CreateTextSelectPlate(&gui->gyro[2], FIELD_X, y, FIELD_WIDTH2, LINE_HEIGHT, &TEXTSEL_FONT, NULL, gyro_val_cb, (void *)2);
    }

    GUI_Select1stSelectableObj();
}

#endif //HAS_STANDARD_GUI
