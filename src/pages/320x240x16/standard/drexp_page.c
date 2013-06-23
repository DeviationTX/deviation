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
    int count = get_mixers();
    int expected = INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_DREXP_AIL+drexp_type]);
    if (count != expected) {
        GUI_CreateLabelBox(&gui->msg, 0, 120, 240, 16, &NARROW_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }
    #define COL3 (4 + ((LCD_WIDTH - 320) / 2))
    #define COL4 (112 + ((LCD_WIDTH - 320) / 2))
    #define COL5 (216 + ((LCD_WIDTH - 320) / 2))
    #define COL1 COL4
    #define COL2 (COL1 + 32)
    #define ROW1 (36 + ((LCD_HEIGHT - 240) / 2))
    #define ROW2 (ROW1 + 20)
    #define ROW3 (ROW1 + 40)
    #define ROW4 (ROW1 + 60)
    #define ROW5 (ROW1 + 80)
    /* Row 1 */
    GUI_CreateLabelBox(&gui->srclbl, COL1, ROW1, 96, 16, &DEFAULT_FONT, NULL, NULL, _tr("Src"));
    GUI_CreateTextSelect(&gui->src, COL2, ROW1, TEXTSELECT_64, NULL, set_type_cb, NULL);
    /* Row 2 */
    GUI_CreateLabelBox(&gui->mode[0], COL3, ROW2, 96, 16, &DEFAULT_FONT, NULL, NULL, STDMIX_ModeName(PITTHROMODE_NORMAL));
    GUI_CreateLabelBox(&gui->mode[1], COL4, ROW2, 96, 16, &DEFAULT_FONT, NULL, NULL, STDMIX_ModeName(PITTHROMODE_IDLE1));
    GUI_CreateLabelBox(&gui->mode[2], COL5, ROW2, 96, 16, &DEFAULT_FONT, NULL, NULL, STDMIX_ModeName(PITTHROMODE_IDLE2));
    /* Row 3 */
    GUI_CreateTextSelect(&gui->dr[0], COL3, ROW3, TEXTSELECT_96, NULL, set_dr_cb, ((void *)(long)PITTHROMODE_NORMAL));
    GUI_CreateTextSelect(&gui->dr[1], COL4, ROW3, TEXTSELECT_96, NULL, set_dr_cb, ((void *)(long)PITTHROMODE_IDLE1));
    GUI_CreateTextSelect(&gui->dr[2], COL5, ROW3, TEXTSELECT_96, NULL, set_dr_cb, ((void *)(long)PITTHROMODE_IDLE2));
    /* Row 4 */
    GUI_CreateTextSelect(&gui->exp[0], COL3, ROW4, TEXTSELECT_96, NULL, set_exp_cb, ((void *)(long)PITTHROMODE_NORMAL));
    GUI_CreateTextSelect(&gui->exp[1], COL4, ROW4, TEXTSELECT_96, NULL, set_exp_cb, ((void *)(long)PITTHROMODE_IDLE1));
    GUI_CreateTextSelect(&gui->exp[2], COL5, ROW4, TEXTSELECT_96, NULL, set_exp_cb, ((void *)(long)PITTHROMODE_IDLE2));
    /* Row 5 */
    u16 ymax = CHAN_MAX_VALUE/100 * MAX_SCALAR;
    s16 ymin = -ymax;
    GUI_CreateXYGraph(&gui->graph[0], COL3, ROW5, 96, 120,
                              CHAN_MIN_VALUE, ymin,
                              CHAN_MAX_VALUE, ymax,
                              0, 0, show_curve_cb, curpos_cb, NULL, (void *)(PITTHROMODE_NORMAL+1L));
    GUI_CreateXYGraph(&gui->graph[1], COL4, ROW5, 96, 120,
                              CHAN_MIN_VALUE, ymin,
                              CHAN_MAX_VALUE, ymax,
                              0, 0, show_curve_cb, curpos_cb, NULL, (void *)(PITTHROMODE_IDLE1+1L));
    GUI_CreateXYGraph(&gui->graph[2], COL5, ROW5, 96, 120,
                              CHAN_MIN_VALUE, ymin,
                              CHAN_MAX_VALUE, ymax,
                              0, 0, show_curve_cb, curpos_cb, NULL, (void *)(PITTHROMODE_IDLE2+1L));
    _refresh_page();
}

static void _refresh_page() {
    int hide3 = (INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_DREXP_AIL+drexp_type]) < 3) ? 1 : 0;
    GUI_SetHidden((guiObject_t *)&gui->mode[2], hide3);
    GUI_SetHidden((guiObject_t *)&gui->dr[2], hide3);
    GUI_SetHidden((guiObject_t *)&gui->exp[2], hide3);
    GUI_SetHidden((guiObject_t *)&gui->graph[2], hide3);

    GUI_RedrawAllObjects();
}

