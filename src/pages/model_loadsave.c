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

static struct model_page * const mp = &pagemem.u.model_page;

static int ini_handle_icon(void* user, const char* section, const char* name, const char* value)
{
    (void)user;
    if(section[0] == '\0' && strcasecmp(name, MODEL_ICON) == 0) {
        CONFIG_ParseModelName(mp->iconstr, value);
    }
    if(section[0] == '\0' && strcasecmp(name, MODEL_TYPE) == 0) {
        mp->modeltype = CONFIG_ParseModelType(value);
    }
    return 1;
}

static int ini_handle_name(void* user, const char* section, const char* name, const char* value)
{
    long idx = (long)user;
    if(section[0] == '\0' && strcasecmp(name, MODEL_NAME) == 0) {
        sprintf(mp->tmpstr, "%d: %s", (int)idx, value);
        return -1;
    }
    return 1;
}

static void select_cb(guiObject_t *obj, u16 sel, void *data)
{
    (void)obj;
    (void)data;
    const char *ico;
    mp->selected = sel + 1;
    GUI_RemoveObj(mp->icon);
    sprintf(mp->tmpstr, "models/model%d.ini", mp->selected);
    mp->modeltype = 0;
    mp->iconstr[0] = 0;
    ini_parse(mp->tmpstr, ini_handle_icon, NULL);
    if (mp->iconstr[0])
        ico = mp->iconstr;
    else
        ico = CONFIG_GetIcon(mp->modeltype);
    mp->icon = GUI_CreateImage(10, 88, 96, 96, ico);
}
static const char *string_cb(u8 idx, void *data)
{
    (void)data;
    FILE *fh;
    sprintf(mp->tmpstr, "models/model%d.ini", idx + 1);
    fh = fopen(mp->tmpstr, "r");
    sprintf(mp->tmpstr, "%d: NONE", idx + 1);
    if (fh)
        ini_parse_file(fh, ini_handle_name, (void *)((long)(idx + 1)));
    return mp->tmpstr;
}
static void okcancel_cb(guiObject_t *obj, void *data)
{
    int msg = (long)data;
    (void)obj;
    if (msg == 1) {
        CONFIG_SaveModelIfNeeded();
        CONFIG_ReadModel(mp->selected);
    } else if (msg == 2) {
        CONFIG_WriteModel(mp->selected);
        CONFIG_ReadModel(mp->selected);  //Reload the model after saving to switch (for future saves)
    }
    GUI_RemoveAllObjects();
    PAGE_ModelInit(0);
}

void MODELPage_ShowLoadSave(int loadsave)
{
    u8 num_models;
    GUI_RemoveAllObjects();
    GUI_CreateButton(160, 4, BUTTON_96, "Cancel", 0x0000, okcancel_cb, (void *)0);
    GUI_CreateButton(264, 4, BUTTON_48, loadsave ? "Save" : "Load", 0x0000, okcancel_cb, (void *)(loadsave+1L));
    for (num_models = 1; num_models <= 100; num_models++) {
        sprintf(mp->tmpstr, "models/model%d.ini", num_models);
        FILE *fh = fopen(mp->tmpstr, "r");
        if (! fh)
            break;
    }
    num_models--;
    mp->selected = CONFIG_GetCurrentModel();
    GUI_CreateListBox(120, 40, 198, 192, num_models, mp->selected-1, string_cb, select_cb, NULL, NULL);
    mp->icon = GUI_CreateImage(10, 88, 96, 96, CONFIG_GetCurrentIcon());
}
