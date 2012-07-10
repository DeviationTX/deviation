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
#include "config/ini.h"

static char file[30];
static int selected;
static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    long idx = (long)user;
    if(section[0] == '\0' && strcasecmp(name, "name") == 0) {
        sprintf(file, "%d: %s", (int)idx, value);
        return -1;
    }
    return 1;
}

static void select_cb(guiObject_t *obj, u16 sel, void *data)
{
    (void)obj;
    (void)data;
    selected = sel + 1;
}
static const char *string_cb(u8 idx, void *data)
{
    (void)data;
    FILE *fh;
    sprintf(file, "models/model%d.ini", idx + 1);
    fh = fopen(file, "r");
    sprintf(file, "%d: NONE", idx + 1);
    if (fh)
        ini_parse_file(fh, ini_handler, (void *)((long)(idx + 1)));
    return file;
}
static void okcancel_cb(guiObject_t *obj, void *data)
{
    int msg = (long)data;
    (void)obj;
    if (msg == 1) {
        CONFIG_SaveModelIfNeeded();
        CONFIG_ReadModel(selected);
    } else if (msg == 2) {
        CONFIG_WriteModel(selected);
        CONFIG_ReadModel(selected);  //Reload the model after saving to switch (for future saves)
    }
    GUI_RemoveAllObjects();
    PAGE_ModelInit(0);
}

void MODELPage_ShowLoadSave(int loadsave)
{
    u8 num_models;
    GUI_RemoveAllObjects();
    GUI_CreateButton(150, 6, BUTTON_90, "Cancel", 0x0000, okcancel_cb, (void *)0);
    GUI_CreateButton(264, 6, BUTTON_45, loadsave ? "Save" : "Load", 0x0000, okcancel_cb, (void *)(loadsave+1L));
    for (num_models = 1; num_models <= 100; num_models++) {
        sprintf(file, "models/model%d.ini", num_models);
        FILE *fh = fopen(file, "r");
        if (! fh)
            break;
    }
    num_models--;
    selected = CONFIG_GetCurrentModel();
    GUI_CreateListBox(110, 40, 100, 190, num_models, selected-1, string_cb, select_cb, NULL, NULL);
}
