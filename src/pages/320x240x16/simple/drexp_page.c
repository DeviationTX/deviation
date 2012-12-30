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
#include "mixer_simple.h"
#include "simple.h"
#include "../../common/simple/_drexp_page.c"

guiObject_t *scroll_bar;

void PAGE_DrExpInit(int page)
{
    (void)page;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_DREXP), MODELMENU_Show);
    memset(mp, 0, sizeof(*mp));
    get_mixers();
    if (!mp->mixer_ptr[0] || !mp->mixer_ptr[1] || !mp->mixer_ptr[2]) {
        GUI_CreateLabelBox(0, 120, 240, 16, &NARROW_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }
    /* Row 1 */
    GUI_CreateLabelBox(10, 40, 96, 16, &DEFAULT_FONT, NULL, NULL, _tr("Src"));
    GUI_CreateTextSelect(100, 40, TEXTSELECT_96, 0x0000, NULL, set_type_cb, NULL);
    /* Row 2 */
    GUI_CreateLabelBox(4, 60, 96, 16, &DEFAULT_FONT, NULL, NULL, SIMPLEMIX_ModeName(PITTHROMODE_NORMAL));
    GUI_CreateLabelBox(112, 60, 96, 16, &DEFAULT_FONT, NULL, NULL, SIMPLEMIX_ModeName(PITTHROMODE_IDLE1));
    GUI_CreateLabelBox(216, 60, 96, 16, &DEFAULT_FONT, NULL, NULL, SIMPLEMIX_ModeName(PITTHROMODE_IDLE2));
    /* Row 3 */
    GUI_CreateTextSelect(4, 80, TEXTSELECT_96, 0x0000, NULL, set_dr_cb, ((void *)(long)PITTHROMODE_NORMAL));
    GUI_CreateTextSelect(112, 80, TEXTSELECT_96, 0x0000, NULL, set_dr_cb, ((void *)(long)PITTHROMODE_IDLE1));
    GUI_CreateTextSelect(216, 80, TEXTSELECT_96, 0x0000, NULL, set_dr_cb, ((void *)(long)PITTHROMODE_IDLE2));
    /* Row 4 */
    GUI_CreateTextSelect(4, 100, TEXTSELECT_96, 0x0000, NULL, set_exp_cb, ((void *)(long)PITTHROMODE_NORMAL));
    GUI_CreateTextSelect(112, 100, TEXTSELECT_96, 0x0000, NULL, set_exp_cb, ((void *)(long)PITTHROMODE_IDLE1));
    GUI_CreateTextSelect(216, 100, TEXTSELECT_96, 0x0000, NULL, set_exp_cb, ((void *)(long)PITTHROMODE_IDLE2));
    /* Row 5 */
    u16 ymax = CHAN_MAX_VALUE/100 * MAX_SCALAR;
    s16 ymin = -ymax;
    mp->graphs[0] = GUI_CreateXYGraph(4, 140, 96, 96,
                              CHAN_MIN_VALUE, ymin,
                              CHAN_MAX_VALUE, ymax,
                              0, 0, show_curve_cb, curpos_cb, NULL, (void *)(long)(PITTHROMODE_NORMAL));
    mp->graphs[1] = GUI_CreateXYGraph(112, 140, 96, 96,
                              CHAN_MIN_VALUE, ymin,
                              CHAN_MAX_VALUE, ymax,
                              0, 0, show_curve_cb, curpos_cb, NULL, (void *)(long)(PITTHROMODE_IDLE1));
    mp->graphs[2] = GUI_CreateXYGraph(216, 140, 96, 96,
                              CHAN_MIN_VALUE, ymin,
                              CHAN_MAX_VALUE, ymax,
                              0, 0, show_curve_cb, curpos_cb, NULL, (void *)(long)(PITTHROMODE_IDLE2));
}

static void _refresh_page() {
    GUI_RedrawAllObjects();
}

