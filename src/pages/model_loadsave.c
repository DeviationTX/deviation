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
    sprintf(mp->tmpstr, "models/model%d.ini", mp->selected);
    mp->modeltype = 0;
    mp->iconstr[0] = 0;
    ini_parse(mp->tmpstr, ini_handle_icon, NULL);
    if (mp->iconstr[0])
        ico = mp->iconstr;
    else
        ico = CONFIG_GetIcon(mp->modeltype);
    GUI_ChangeImage(mp->icon, ico, 0, 0);
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
    mp->return_page(0);
}

const char *show_loadsave_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    return ((long)data) == 2 ? "Save" : "Load";
}

void MODELPage_ShowLoadSave(int loadsave, void(*return_page)(int page))
{
    u8 num_models;
    GUI_RemoveAllObjects();
    PAGE_SetModal(1);
    mp->return_page = return_page;
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    GUI_CreateButton(264, 4, BUTTON_48, show_loadsave_cb, 0x0000, okcancel_cb, (void *)(loadsave+1L));
    for (num_models = 1; num_models <= 100; num_models++) {
        sprintf(mp->tmpstr, "models/model%d.ini", num_models);
        FILE *fh = fopen(mp->tmpstr, "r");
        if (! fh)
            break;
    }
    num_models--;
    mp->selected = CONFIG_GetCurrentModel();
    GUI_CreateListBox(112, 40, 200, 192, num_models, mp->selected-1, string_cb, select_cb, NULL, NULL);
    mp->icon = GUI_CreateImage(8, 88, 96, 96, CONFIG_GetCurrentIcon());
}
