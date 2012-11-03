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

#include "../common/_usb_page.c"

static void _draw_page(u8 enable)
{
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr("USB"));

    sprintf(up->tmpstr, "%s %s\n%s %s",
            _tr("Deviation FW\nversion:"), DeviationVersion,
            _tr("Press 'Ent' to turn USB Filesystem:"),
            enable == 0 ? _tr("On") : _tr("Off"));
    GUI_CreateLabelBox(20, 80, 280, 100, &NARROW_FONT, NULL, NULL, up->tmpstr);
}

