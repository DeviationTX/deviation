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

static struct model_page * const mp = &pagemem.u.model_page;
static struct modelload_obj * const gui = &gui_objs.u.modelload;

static int ini_handle_icon(void* user, const char* section, const char* name, const char* value)
{
    (void)user;
    if(section[0] == '\0' && strcasecmp(name, MODEL_ICON) == 0) {
        CONFIG_ParseIconName(mp->iconstr, value);
    }
    if(section[0] == '\0' && strcasecmp(name, MODEL_TYPE) == 0) {
        mp->modeltype = CONFIG_ParseModelType(value);
    }
    return 1;
}

static int ini_handle_name(void* user, const char* section, const char* name, const char* value)
{
    long idx = (long)user;
    if(section[0] == '\0' && (strcasecmp(name, MODEL_NAME) == 0 || strcasecmp(name, MODEL_TEMPLATE) == 0)) {
        snprintf(tempstring, sizeof(tempstring), "%d: %s", abs(idx), idx < 0 ? _tr(value) : value);
        return -1;
    }
    return 1;
}

static void change_icon(int sel)
{
    const char *ico;
    if(! OBJ_IS_USED(&gui->image))
        return;
    if (mp->menu_type == LOAD_ICON) {
        ico = CONFIG_GetIcon(mp->modeltype);
        if (sel > 0 && FS_OpenDir("modelico")) {
            char filename[13];
            int count = 0;
            int type;
            while((type = FS_ReadDir(filename)) != 0) {
                if (type == 1 && strncasecmp(filename + strlen(filename) - 4, IMG_EXT, 4) == 0) {
                    count++;
                    if (sel == count) {
                        CONFIG_ParseIconName(mp->iconstr, filename);
                        ico = mp->iconstr;
                        break;
                    }
                }
            }
            FS_CloseDir();
        }
    } else {
        sel++; //models are indexed from 1
        sprintf(tempstring, "models/model%d.ini", sel);
        mp->modeltype = 0;
        mp->iconstr[0] = 0;
        ini_parse(tempstring, ini_handle_icon, NULL);
        if (sel == CONFIG_GetCurrentModel() && Model.icon[0])
            ico = Model.icon;
        else {
            if (mp->iconstr[0])
                ico = mp->iconstr;
            else
                ico = CONFIG_GetIcon(mp->modeltype);
        }
        if (! fexists(ico))
            ico = UNKNOWN_ICON;
    }
    GUI_ReplaceImage(&gui->image, ico, 0, 0);
}

int get_idx_filename(char *result, const char *dir, const char *ext, int idx, const char *prefix)
{
    if (! FS_OpenDir(dir))
        return 0;
    char filename[13];
    int type;
    int count = 0;
    while((type = FS_ReadDir(filename)) != 0) {
        if (type == 1 && strncasecmp(filename + strlen(filename) - 4, ext, 4) == 0) {
            count++;
            if (idx + 1 == count) {
                sprintf(result, "%s%s", prefix, filename);
                FS_CloseDir();
                return 1;
            }
        }
    }
    FS_CloseDir();
    return 0;
}
static const char *name_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long idx = (long)data;
    FILE *fh;
    if (mp->menu_type == LOAD_TEMPLATE) { //Template
        if (! get_idx_filename(tempstring, "template", ".ini", idx, "template/"))
            return _tr("Unknown");
    } else if (mp->menu_type == LOAD_ICON) { //Icon
        if (idx == 0)
            return _tr("Default");
        if (! get_idx_filename(tempstring, "modelico", IMG_EXT, idx-1, ""))
            return _tr("Unknown");
        return tempstring;
    } else if (mp->menu_type == LOAD_LAYOUT) {
        if (idx >= mp->file_state)
            sprintf(tempstring, "models/model%d.ini", idx + 1 - mp->file_state);
        else
            if (! get_idx_filename(tempstring, "layout", ".ini", idx, "layout/"))
                return _tr("Unknown");
    } else {
        if (idx + 1 == CONFIG_GetCurrentModel()) {
            sprintf(tempstring, "%d: %s%s", idx + 1, Model.name, CONFIG_IsModelChanged() ? " (unsaved)" : "");
            return tempstring;
        }
        sprintf(tempstring, "models/model%d.ini", idx + 1);
    }
    fh = fopen(tempstring, "r");
    sprintf(tempstring, "%d: NONE", idx + 1);
    if (fh) {
        long user = idx + 1;
        if (mp->menu_type == LOAD_TEMPLATE)
            user = -user;
        ini_parse_file(fh, ini_handle_name, (void *)user);
        fclose(fh);
    }
    if (mp->menu_type == LOAD_LAYOUT && idx >= mp->file_state)
        strcat(tempstring + strlen(tempstring), "(M)");
    return tempstring;
}

int model_count()
{
    int num_models;
    for (num_models = 1; num_models <= 255; num_models++) {
        sprintf(tempstring, "models/model%d.ini", num_models);
        FILE *fh = fopen(tempstring, "r");
        if (! fh)
            break;
        fclose(fh);
    }
    num_models--;
    return num_models;
}

/*count will be in mp->total_items. Return is selection if any */
static int count_files(const char *dir, const char *ext, const char *match)
{
    int num_files = 0;
    int selected = 0;
    if (FS_OpenDir(dir)) {
        char filename[13];
        int type;
        while((type = FS_ReadDir(filename)) != 0) {
            if (type == 1 && strncasecmp(filename + strlen(filename) - 4, ext, 4) == 0) {
                num_files++;
                if(match && strncasecmp(match, filename, 13) == 0) {
                    selected = num_files;
                }
            }
        }
        FS_CloseDir();
    }
    mp->total_items = num_files;
    return selected;
}

static void press_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    int selected = (long)data + 1;
    if (press_type != -1)
        return;

    if (mp->menu_type == LOAD_MODEL) {
        /* Load Model */
        if (selected != CONFIG_GetCurrentModel()) { // don't do that if model didn't change
            CONFIG_SaveModelIfNeeded();
            PROTOCOL_DeInit();
            CONFIG_ReadModel(selected);
            CONFIG_SaveTxIfNeeded();  //Save here to ensure in case of crash we restart on the right model
            /* Need to recalculate channels to see if we're in a safe state */
            MIXER_Init();
            MIXER_CalcChannels();
            PROTOCOL_Init(0);
        }
    } else if (mp->menu_type == SAVE_MODEL) {
        /* Save Model */
        CONFIG_WriteModel(selected);
        CONFIG_ReadModel(selected);  //Reload the model after saving to switch (for future saves)
    } else if (mp->menu_type == LOAD_TEMPLATE) {
        /* Load Template */
        get_idx_filename(tempstring, "template", ".ini", selected-1, "");
        CONFIG_ReadTemplate(tempstring);
    } else if (mp->menu_type == LOAD_ICON) {
        if (selected == 1)
            Model.icon[0] = 0;
        else
            strcpy(Model.icon, mp->iconstr);
    } else if (mp->menu_type == LOAD_LAYOUT) {
        /* Load Layout */
        if (selected > mp->file_state) {
            sprintf(tempstring, "models/model%d.ini", selected - mp->file_state);
        } else {
            get_idx_filename(tempstring, "layout", ".ini", selected-1, "layout/");
        }
        CONFIG_ReadLayout(tempstring);
    }
    PAGE_Pop();
}

static int get_scroll_count(enum loadSaveType p)
{
    int selected = 0;
    switch(p) {
      case LOAD_MODEL:
      case SAVE_MODEL:
        mp->total_items = model_count();
        selected = CONFIG_GetCurrentModel();
        break;
      case LOAD_TEMPLATE:
        selected = count_files("template", ".ini", NULL);
        break;
      case LOAD_ICON:
        strlcpy(mp->iconstr, CONFIG_GetIcon(Model.type), sizeof(mp->iconstr));
        selected = count_files("modelico", IMG_EXT, Model.icon[0] ? Model.icon+9 : NULL);
        selected++; //Selected is actually accurate, so we increment here to decrement below
        mp->total_items++; //Default is 1st
        break;
      case LOAD_LAYOUT:
        selected = count_files("layout", ".ini", "default.ini");
        mp->file_state = mp->total_items;
        mp->total_items += model_count();
        break;
    }
    if (selected > 0)
        selected--;
    return selected;
}
