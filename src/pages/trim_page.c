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

#include "target.h"
#include "pages.h"
#include "gui/gui.h"

static struct trim_page * const tp = &pagemem.u.trim_page;

const char *show_int_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    long num = (long)data;
    sprintf(tp->tmpstr, "%ld", num);
    return tp->tmpstr;
}

const char *trimsource_name_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    u8 i = (long)data;
    struct Trim *trim = MIX_GetAllTrims();
    return MIXER_SourceName(tp->tmpstr, trim[i].src);
}

const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    *source = GUI_TextSelectHelper(MIX_SRC(*source), 0, NUM_INPUTS + NUM_CHANNELS, dir, 1, 1, NULL);
    return MIXER_SourceName(tp->tmpstr, *source);
}

const char *set_trim_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *button = (u8 *)data;
    *button = GUI_TextSelectHelper(*button, 0, NUM_TX_BUTTONS, dir, 1, 1, NULL);
    return MIXER_ButtonName(*button);
}

static void okcancel_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if (data) {
        //Save trim here
        struct Trim *trim = MIX_GetAllTrims();
        trim[tp->index] = tp->trim;
    }
    GUI_RemoveAllObjects();
    PAGE_TrimInit(0);
}
void edit_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    struct Trim *trim = MIX_GetAllTrims();
    PAGE_SetModal(1);
    tp->index = (long)data;
    tp->trim = trim[tp->index];
   
    GUI_RemoveAllObjects();
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);

    //Row 1
    GUI_CreateLabel(8, 48, NULL, DEFAULT_FONT, "Input:");
    GUI_CreateTextSelect(72, 48, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &tp->trim.src);
    //Row 2
    GUI_CreateLabel(8, 72, NULL, DEFAULT_FONT, "Trim -:");
    GUI_CreateTextSelect(72, 72, TEXTSELECT_96, 0x0000, NULL, set_trim_cb, &tp->trim.neg);
    GUI_CreateLabel(176, 72, NULL, DEFAULT_FONT, "Trim +:");
    GUI_CreateTextSelect(216, 72, TEXTSELECT_96, 0x0000, NULL, set_trim_cb, &tp->trim.pos);
    //Row 3
    GUI_CreateLabel(8, 96, NULL, DEFAULT_FONT, "Trim Step:");
    GUI_CreateTextSelect(72, 96, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &tp->trim.step);
}

void PAGE_TrimInit(int page)
{
    (void)page;
    int i;
    PAGE_SetModal(0);
    PAGE_ShowHeader("Trim");

    GUI_CreateLabel(8, 40, NULL, DEFAULT_FONT, "Input");
    GUI_CreateLabel(72, 40, NULL, DEFAULT_FONT, "Trim Neg");
    GUI_CreateLabel(136, 40, NULL, DEFAULT_FONT, "Trim Pos");
    GUI_CreateLabel(200, 40, NULL, DEFAULT_FONT, "Trim Step");
    struct Trim *trim = MIX_GetAllTrims();
    for (i = 0; i < NUM_TRIMS; i++) {
        GUI_CreateButton(8, 24*i + 64, BUTTON_48x16,
            trimsource_name_cb, 0x0000, edit_cb, (void *)((long)i));
        GUI_CreateLabel(72, 24*i + 66, NULL, DEFAULT_FONT, (void *)MIXER_ButtonName(trim[i].neg));
        GUI_CreateLabel(136, 24*i + 66, NULL, DEFAULT_FONT, (void *)MIXER_ButtonName(trim[i].pos));
        GUI_CreateTextSelect(200, 24*i + 64, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &trim[i].step);
    }
}

void PAGE_TrimEvent()
{
}

