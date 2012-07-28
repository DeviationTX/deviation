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
#include "config/model.h"

static const char *swash_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    Model.swash_type = GUI_TextSelectHelper(Model.swash_type, 0 , SWASH_TYPE_90, dir, 1, 1, NULL);
    switch(Model.swash_type) {
        case SWASH_TYPE_NONE: return "None";
        case SWASH_TYPE_120:  return "120";
        case SWASH_TYPE_120X: return "120X";
        case SWASH_TYPE_140:  return "140";
        case SWASH_TYPE_90:   return "90";
    }
    return "";
}

static const char *swashinv_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    Model.swash_invert = GUI_TextSelectHelper(Model.swash_invert, 0 , 1, dir, 1, 1, NULL);
    switch(Model.swash_invert) {
        case 0: return "Normal";
        case 1: return "Inverted";
    }
    return "";
}
void swashinv_press_cb(guiObject_t *obj, void *data)
{
    (void)data;
    Model.swash_invert ^= 1;
    GUI_Redraw(obj);
}

void okcancel_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    GUI_RemoveAllObjects();
    PAGE_ModelInit(0);
}

static void show_titlerow(const char *header)
{
    GUI_CreateLabel(8, 10, NULL, TITLE_FONT, (void *)header);
    PAGE_CreateOkButton(264, 4, okcancel_cb);
}

void MODELPAGE_Config()
{
    PAGE_SetModal(1);
    show_titlerow(Model.type == 0 ? "Helicopter" : "Airplane");
    if (Model.type == 0) {
        GUI_CreateLabel(8, 40, NULL, DEFAULT_FONT, "SwashType:");
        GUI_CreateTextSelect(136, 40, TEXTSELECT_96, 0x0000, NULL, swash_val_cb, NULL);
        GUI_CreateLabel(8, 64, NULL, DEFAULT_FONT, "SwashInv:");
        GUI_CreateTextSelect(136, 64, TEXTSELECT_96, 0x0000, swashinv_press_cb, swashinv_val_cb, NULL);
    }
}
