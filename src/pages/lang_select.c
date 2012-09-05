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
#include <string.h>

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
    char filename[13];
    int type;
    if (idx == 0) {
        return "English";
    }
    if (! FS_OpenDir("language"))
        return _tr("Unknown");
    while((type = FS_ReadDir(filename)) != 0) {
        if (type == 1 && strncasecmp(filename, "lang", 4) == 0) {
            idx--;
            if (idx == 0) {
                sprintf(cp.tmpstr, "language/%s", filename);
                break;
            }
        }
    }
    FS_CloseDir();
    if(idx == 0) {
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
    num_lang = 1;
    if (FS_OpenDir("language")) {
        char filename[13];
        int type;
        while((type = FS_ReadDir(filename)) != 0) {
            if (type == 1 && strncasecmp(filename, "lang", 4) == 0) {
                num_lang++;
            }
        }
        FS_CloseDir();
    }
    cp.selected = Transmitter.language;
    GUI_CreateListBox(112, 40, 200, 192, num_lang, cp.selected, string_cb, select_cb, NULL, NULL);
}
