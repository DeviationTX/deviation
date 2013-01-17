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
#include "config/model.h"
#include "config/ini.h"
#include <stdlib.h>

#include "../common/_model_loadsave.c"

static void _show_buttons(int loadsave)
{
    PAGE_CreateCancelButton(112, 4, okcancel_cb);
    GUI_CreateButton(&gui->ok, 216, 4, BUTTON_96, show_loadsave_cb, 0x0000, okcancel_cb, (void *)(loadsave+1L));
}

static void _show_list(int loadsave, u8 num_models)
{
    GUI_CreateListBox(&gui->list, 112, 40, 200, 192, num_models, mp->selected-1, string_cb, select_cb, NULL, (void *)(long)loadsave);
    if (loadsave != LOAD_TEMPLATE)
        GUI_CreateImage(&gui->image, 8, 88, 96, 96, mp->iconstr);
}
