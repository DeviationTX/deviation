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
#include "config/tx.h"

#define cp (pagemem.u.calibrate_page)

static void select_cb(guiObject_t *obj, u16 sel, void *data)
{
    (void)obj;
    (void)data;
    cp.selected = sel;
}

static const char *string_cb(u8 idx, void *data)
{
    (void)data;
    FILE *fh;
    if (idx == 0) {
        return "English";
    }
    sprintf(cp.tmpstr, "language/lang%d.txt", idx);
    fh = fopen(cp.tmpstr, "r");
    if (fh) {
        fgets(cp.tmpstr, sizeof(cp.tmpstr), fh);
        fclose(fh);
        unsigned len = strlen(cp.tmpstr);
        if(strlen(cp.tmpstr) && cp.tmpstr[0] != ':') {
            cp.tmpstr[len-1] = '\0';
            return cp.tmpstr;
        }
    }
    return _tr("Unknown");
}
static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    if (data)
        CONFIG_ReadLang(cp.selected);
    GUI_RemoveAllObjects();
    cp.return_page(0);
}

static const char *show_load_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return _tr("Load");
}

void LANGPage_Select(void(*return_page)(int page))
{
    u8 num_lang;
    PAGE_RemoveAllObjects();
    PAGE_SetModal(1);
    cp.return_page = return_page;
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    GUI_CreateButton(264, 4, BUTTON_48, show_load_cb, 0x0000, okcancel_cb, (void *)1L);
    for (num_lang = 1; num_lang <= 100; num_lang++) {
        sprintf(cp.tmpstr, "language/lang%d.txt", num_lang);
        FILE *fh = fopen(cp.tmpstr, "r");
        if (! fh)
            break;
        fclose(fh);
    }
    cp.selected = Transmitter.language;
    GUI_CreateListBox(112, 40, 200, 192, num_lang, cp.selected, string_cb, select_cb, NULL, NULL);
}
