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
#include "gui/gui.h"
#include "config/model.h"
#include "../pages.h"
#include "standard.h"

#include "../../common/standard/_common_standard.c"

int STDMIX_ScrollCB(guiObject_t *parent, u8 pos, s8 direction, void *data)
{
    (void)parent;
    struct mixer_page * _mp = &pagemem.u.mixer_page;
    void (*show_page_cb)(int) = data;
    s16 newpos;
    if (direction > 0) {
        newpos = pos + (direction > 1 ? ENTRIES_PER_PAGE : 1);
        if (newpos > _mp->max_scroll)
            newpos = _mp->max_scroll;
    } else {
        newpos = pos - (direction < -1 ? ENTRIES_PER_PAGE : 1);
        if (newpos < 0)
            newpos = 0;
    }
    if (newpos != pos)
        show_page_cb(newpos);
    return newpos;
}

