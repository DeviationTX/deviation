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
#include "../pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "mixer.h"
#include "simple.h"
#include "mixer_simple.h"
#include "../../common/simple/_gyrosense_page.c"

void PAGE_GyroSenseInit(int page)
{
    (void)page;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_GYROSENSE), MODELMENU_Show);
    memset(mp, 0, sizeof(*mp));
    SIMPLEMIX_GetMixers(mp->mixer_ptr, mapped_simple_channels.aux2, GYROMIXER_COUNT);
    if (!mp->mixer_ptr[0] || !mp->mixer_ptr[1] || !mp->mixer_ptr[2])  // should be switched to gear
        SIMPLEMIX_GetMixers(mp->mixer_ptr, mapped_simple_channels.gear, GYROMIXER_COUNT);
    if (!mp->mixer_ptr[0] || !mp->mixer_ptr[1] || !mp->mixer_ptr[2]) {
        GUI_CreateLabelBox(0, 120, 240, 16, &NARROW_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }
    gryo_output = mp->mixer_ptr[0]->dest;
    convert_output_to_percentile();

    /* Row 1 */
    GUI_CreateLabelBox(10, 40, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("Channel"));
    GUI_CreateTextSelect(120, 40, TEXTSELECT_128, 0x0000, NULL, gyro_output_cb, NULL);

    /* Row 2 */
    GUI_CreateLabelBox(10, 60, 0, 16, &DEFAULT_FONT, label_cb, NULL, (void *)1L);
    GUI_CreateTextSelect(120, 60, TEXTSELECT_128, 0x0000, NULL, gyro_val_cb, (void *)0L);

    /* Row 3 */
    GUI_CreateLabelBox(10, 80, 0, 16, &DEFAULT_FONT, label_cb, NULL, (void *)2L);
    GUI_CreateTextSelect(120, 80, TEXTSELECT_128, 0x0000, NULL, gyro_val_cb, (void *)1);

    /* Row 4 */
    GUI_CreateLabelBox(10, 100, 0, 16, &DEFAULT_FONT, label_cb, NULL, (void *)3L);
    GUI_CreateTextSelect(120, 100, TEXTSELECT_128, 0x0000, NULL, gyro_val_cb, (void *)2);
}
