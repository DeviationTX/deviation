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
    mp->gyro_output = mp->mixer_ptr[0]->dest;
    convert_output_to_percentile();
    PAGE_ShowHeader(_tr_noop("Gyro sense"));

    u8 y = HEADER_HEIGHT + HEADER_OFFSET;
    GUI_CreateLabelBox(&gui->chanlbl, LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Channel"));
    GUI_CreateTextSelectPlate(&gui->chan, FIELD_X, y, FIELD_WIDTH1, LINE_HEIGHT, &TEXTSEL_FONT, NULL, gyro_output_cb, NULL);

    for (int i=0; i < ((INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_GYROSENSE]) == 3) ? 3 : 2); i++) {
        y += LINE_SPACE;
        GUI_CreateLabelBox(&gui->gyrolbl[i], LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, label_cb, NULL, (void *)(uintptr_t)i);
        GUI_CreateTextSelectPlate(&gui->gyro[i], FIELD_X, y, FIELD_WIDTH2, LINE_HEIGHT, &TEXTSEL_FONT, NULL, gyro_val_cb, (void *)(uintptr_t)i);
    }

    GUI_Select1stSelectableObj();
}

#endif //HAS_STANDARD_GUI
