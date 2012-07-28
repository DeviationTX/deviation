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
#include "template.h"

static struct model_page * const mp = &pagemem.u.model_page;

static void okcancel_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    if (data) {
        TEMPLATE_Apply(mp->selected);
    }
    GUI_RemoveAllObjects();
    PAGE_ModelInit(0);
}

static const char *string_cb(u8 idx, void *data)
{
    (void)data;
    switch(idx) {
        case TEMPLATE_NONE:       return "None";
        case TEMPLATE_4CH_SIMPLE: return "Simple 4Ch";
        case TEMPLATE_4CH_DR:     return "4Ch w/ Dual Rates";
        case TEMPLATE_6CH_PLANE:  return "6Ch Airplane";
        case TEMPLATE_6CH_HELI:   return "6Ch Helicopter";
    }
    return "";
}

static void select_cb(guiObject_t *obj, u16 sel, void *data)
{
    (void)obj;
    (void)data;
    mp->selected = sel;
}

void MODELPage_Template()
{
    GUI_RemoveAllObjects();
    PAGE_SetModal(1);
    mp->selected = 0;
    GUI_CreateLabel(8, 10, NULL, TITLE_FONT, "Templates");
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);
    GUI_CreateListBox(112, 40, 200, 192, NUM_TEMPLATES, 0, string_cb, select_cb, NULL, NULL);
}

