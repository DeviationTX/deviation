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

static void _show_buttons(int loadsave);
static void _show_list(int loadsave, u8 num_models);

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

static void select_cb(guiObject_t *obj, u16 sel, void *data)
{
    (void)obj;
    (void)data;
    const char *ico;
    mp->selected = sel + 1;
    if(! OBJ_IS_USED(&gui->image))
        return;
    if ((long)data == LOAD_ICON) {
        ico = CONFIG_GetIcon(mp->modeltype);
        if (sel > 0 && FS_OpenDir("modelico")) {
            char filename[13];
            int count = 0;
            int type;
            while((type = FS_ReadDir(filename)) != 0) {
                if (type == 1 && strncasecmp(filename + strlen(filename) - 4, ".bmp", 4) == 0) {
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
        sprintf(tempstring, "models/model%d.ini", mp->selected);
        mp->modeltype = 0;
        mp->iconstr[0] = 0;
        ini_parse(tempstring, ini_handle_icon, NULL);
        if (mp->selected == CONFIG_GetCurrentModel() && Model.icon[0])
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
static const char *string_cb(u8 idx, void *data)
{
    (void)data;
    FILE *fh;
    if ((long)data == LOAD_TEMPLATE) { //Template
        if (! get_idx_filename(tempstring, "template", ".ini", idx, "template/"))
            return _tr("Unknown");
    } else if ((long)data == LOAD_ICON) { //Icon
        if (idx == 0)
            return _tr("Default");
        if (! get_idx_filename(tempstring, "modelico", ".bmp", idx-1, ""))
            return _tr("Unknown");
        return tempstring;
    } else if ((long)data == LOAD_LAYOUT) {
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
        if ((long)data == LOAD_TEMPLATE)
            user = -user;
        ini_parse_file(fh, ini_handle_name, (void *)user);
        fclose(fh);
    }
    if ((long)data == LOAD_LAYOUT && idx >= mp->file_state)
        strcat(tempstring + strlen(tempstring), "(M)");
    return tempstring;
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    int msg = (long)data;
    (void)obj;
    if (msg == LOAD_MODEL + 1) {
        /* Load Model */
        if (mp->selected != CONFIG_GetCurrentModel()) { // don't do that if model didn't change
            CONFIG_SaveModelIfNeeded();
            PROTOCOL_DeInit();
            CONFIG_ReadModel(mp->selected);
            CONFIG_SaveTxIfNeeded();  //Save here to ensure in case of crash we restart on the right model
            /* Need to recalculate channels to see if we're in a safe state */
            MIXER_Init();
            MIXER_CalcChannels();
            PROTOCOL_Init(0);
        }
    } else if (msg == SAVE_MODEL + 1) {
        /* Save Model */
        CONFIG_WriteModel(mp->selected);
        CONFIG_ReadModel(mp->selected);  //Reload the model after saving to switch (for future saves)
    } else if (msg == LOAD_TEMPLATE + 1) {
        /* Load Template */
        get_idx_filename(tempstring, "template", ".ini", mp->selected-1, "");
        CONFIG_ReadTemplate(tempstring);
    } else if (msg == LOAD_ICON + 1) {
        if (mp->selected == 1)
            Model.icon[0] = 0;
        else
            strcpy(Model.icon, mp->iconstr);
    } else if (msg == LOAD_LAYOUT + 1) {
        /* Load Layout */
        if (mp->selected > mp->file_state) {
            sprintf(tempstring, "models/model%d.ini", mp->selected - mp->file_state);
        } else {
            get_idx_filename(tempstring, "layout", ".ini", mp->selected-1, "layout/");
        }
        CONFIG_ReadLayout(tempstring);
    }
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    mp->return_page(-1);  // -1 for devo10 means return to the focus of previous page, which is important so that users don't need to scroll down from the 1st item
}

static const char *show_loadsave_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return ((long)data) == SAVE_MODEL + 1 ? _tr("Save") : _tr("Load");
}

int model_count()
{
    int num_models;
    for (num_models = 1; num_models <= 100; num_models++) {
        sprintf(tempstring, "models/model%d.ini", num_models);
        FILE *fh = fopen(tempstring, "r");
        if (! fh)
            break;
        fclose(fh);
    }
    num_models--;
    return num_models;
}

int count_files(const char *dir, const char *ext, const char *match)
{
    int num_files = 0;
    if (FS_OpenDir(dir)) {
        char filename[13];
        int type;
        while((type = FS_ReadDir(filename)) != 0) {
            if (type == 1 && strncasecmp(filename + strlen(filename) - 4, ext, 4) == 0) {
                num_files++;
                if(match && strncasecmp(match, filename, 13) == 0) {
                    mp->selected = num_files;
                }
            }
        }
        FS_CloseDir();
    }
    return num_files;
}
/* loadsave values:
 * 0 : Load Model
 * 1 : Save Model
 * 2 : Load Template
 * 3 : Load Icon
 * 4 : Load Layout
 */
void MODELPage_ShowLoadSave(int loadsave, void(*return_page)(int page))
{
    u8 num_models = 0;
    PAGE_RemoveAllObjects();
    PAGE_SetModal(1);
    mp->return_page = return_page;
    _show_buttons(loadsave);
    if (loadsave == LOAD_TEMPLATE) { //Template
        num_models = count_files("template", ".ini", NULL);
        mp->selected = 1;
    } else if (loadsave == LOAD_ICON) { //Icon
        mp->selected = 0;
        num_models = 1 + count_files("modelico", ".bmp", Model.icon[0] ? Model.icon+9 : NULL);
        const char *ico = mp->selected == 0 ? CONFIG_GetIcon(Model.type) : CONFIG_GetCurrentIcon();
        strncpy(mp->iconstr, ico, sizeof(mp->iconstr));
        mp->selected++;
    } else if (loadsave == LOAD_LAYOUT) { //Layout
        mp->selected = 1;
        num_models = count_files("layout", ".ini", "default.ini");
        mp->file_state = num_models;
        num_models += model_count();
    } else {
        num_models = model_count();
        strncpy(mp->iconstr, CONFIG_GetCurrentIcon(), sizeof(mp->iconstr));
        if (loadsave == SAVE_MODEL)
            mp->selected = 0;
        else
            mp->selected = CONFIG_GetCurrentModel();
    }
    _show_list(loadsave, num_models);
}
