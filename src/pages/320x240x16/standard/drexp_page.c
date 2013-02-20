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
#include "mixer_standard.h"
#include "standard.h"
#include "../../common/standard/_drexp_page.c"

guiObject_t *scroll_bar;

void update_graph(int graph)
{
    GUI_Redraw(&gui->graph[graph]);
}

void PAGE_DrExpInit(int page)
{
    (void)page;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_DREXP), MODELMENU_Show);
    PAGE_ShowHeader_ExitOnly(NULL, MODELMENU_Show);
    PAGE_ShowHeader_SetLabel(STDMIX_TitleString, SET_TITLE_DATA(PAGEID_DREXP, SWITCHFUNC_DREXP_AIL+drexp_type));
    memset(mp, 0, sizeof(*mp));
    get_mixers();
    if (!mp->mixer_ptr[0] || !mp->mixer_ptr[1] || !mp->mixer_ptr[2]) {
        GUI_CreateLabelBox(&gui->msg, 0, 120, 240, 16, &NARROW_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }
    /* Row 1 */
    GUI_CreateLabelBox(&gui->srclbl, 10, 40, 96, 16, &DEFAULT_FONT, NULL, NULL, _tr("Src"));
    GUI_CreateTextSelect(&gui->src, 100, 40, TEXTSELECT_96, NULL, set_type_cb, NULL);
    /* Row 2 */
    GUI_CreateLabelBox(&gui->mode[0], 4, 60, 96, 16, &DEFAULT_FONT, NULL, NULL, STDMIX_ModeName(PITTHROMODE_NORMAL));
    GUI_CreateLabelBox(&gui->mode[1], 112, 60, 96, 16, &DEFAULT_FONT, NULL, NULL, STDMIX_ModeName(PITTHROMODE_IDLE1));
    GUI_CreateLabelBox(&gui->mode[2], 216, 60, 96, 16, &DEFAULT_FONT, NULL, NULL, STDMIX_ModeName(PITTHROMODE_IDLE2));
    /* Row 3 */
    GUI_CreateTextSelect(&gui->dr[0], 4, 80, TEXTSELECT_96, NULL, set_dr_cb, ((void *)(long)PITTHROMODE_NORMAL));
    GUI_CreateTextSelect(&gui->dr[1], 112, 80, TEXTSELECT_96, NULL, set_dr_cb, ((void *)(long)PITTHROMODE_IDLE1));
    GUI_CreateTextSelect(&gui->dr[2], 216, 80, TEXTSELECT_96, NULL, set_dr_cb, ((void *)(long)PITTHROMODE_IDLE2));
    /* Row 4 */
    GUI_CreateTextSelect(&gui->exp[0], 4, 100, TEXTSELECT_96, NULL, set_exp_cb, ((void *)(long)PITTHROMODE_NORMAL));
    GUI_CreateTextSelect(&gui->exp[1], 112, 100, TEXTSELECT_96, NULL, set_exp_cb, ((void *)(long)PITTHROMODE_IDLE1));
    GUI_CreateTextSelect(&gui->exp[2], 216, 100, TEXTSELECT_96, NULL, set_exp_cb, ((void *)(long)PITTHROMODE_IDLE2));
    /* Row 5 */
    u16 ymax = CHAN_MAX_VALUE/100 * MAX_SCALAR;
    s16 ymin = -ymax;
    GUI_CreateXYGraph(&gui->graph[0], 4, 140, 96, 96,
                              CHAN_MIN_VALUE, ymin,
                              CHAN_MAX_VALUE, ymax,
                              0, 0, show_curve_cb, curpos_cb, NULL, (void *)(long)(PITTHROMODE_NORMAL));
    GUI_CreateXYGraph(&gui->graph[1], 112, 140, 96, 96,
                              CHAN_MIN_VALUE, ymin,
                              CHAN_MAX_VALUE, ymax,
                              0, 0, show_curve_cb, curpos_cb, NULL, (void *)(long)(PITTHROMODE_IDLE1));
    GUI_CreateXYGraph(&gui->graph[2], 216, 140, 96, 96,
                              CHAN_MIN_VALUE, ymin,
                              CHAN_MAX_VALUE, ymax,
                              0, 0, show_curve_cb, curpos_cb, NULL, (void *)(long)(PITTHROMODE_IDLE2));
}

static void _refresh_page() {
    GUI_RedrawAllObjects();
}

