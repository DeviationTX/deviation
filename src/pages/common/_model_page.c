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
#define gui (&gui_objs.u.modelpage)
static long callback_result; // Bug fix: u8 is a wrong data type, causing memory violation and unpredictable behavior in real devo10's modelname editing

static void _changename_cb(guiObject_t *obj, const void *data);
static void fixedid_cb(guiObject_t *obj, const void *data);
static void bind_cb(guiObject_t *obj, const void *data);
static void configure_bind_button();
static const char *type_val_cb(guiObject_t *obj, int dir, void *data);
static void type_press_cb(guiObject_t *obj, void *data);
static const char *numchanselect_cb(guiObject_t *obj, int dir, void *data);
static const char *powerselect_cb(guiObject_t *obj, int dir, void *data);
static const char *protoselect_cb(guiObject_t *obj, int dir, void *data);
static const char *file_val_cb(guiObject_t *obj, int dir, void *data);
static void file_press_cb(guiObject_t *obj, void *data);
static void changeicon_cb(guiObject_t *obj, const void *data);

static void _changename_done_cb(guiObject_t *obj, void *data);
static inline guiObject_t *_get_obj(int type, int objid);

enum {
    ITEM_FILE,
    ITEM_NAME,
    ITEM_ICON,
    ITEM_TYPE,
    ITEM_TXPOWER,
    ITEM_PPMIN,
    ITEM_PROTO,
    ITEM_FIXEDID,
    ITEM_NUMCHAN,
#if !defined(NO_STANDARD_GUI)
    ITEM_GUI,
#endif
    ITEM_LAST,
};

const char *show_text_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int width; int height;
    u16 txt_w, txt_h;
    strcpy(mp->tmpstr, (const char *)data);
    GUI_GetSize(obj, &width, &height);
    width -=2;
    while(1) {
        LCD_GetStringDimensions((const u8 *)mp->tmpstr, &txt_w, &txt_h);
        if (txt_w > width) {
            int len = strlen(mp->tmpstr);
            if (mp->tmpstr[len-1] == '.')
                len--;
            mp->tmpstr[len-3] = '.';
            mp->tmpstr[len-2] = '.';
            mp->tmpstr[len-1] = '\0';
        } else {
            break;
        }
    }
    return mp->tmpstr;
}

const char *show_bindtext_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return PROTOCOL_AutoBindEnabled() ? _tr("Re-Init") : _tr("Bind");
}

void PAGE_ModelEvent()
{
}

static void fixedid_done_cb(guiObject_t *obj, void *data)
{
    (void)data;
    if (callback_result == 1) {
        Model.fixed_id = atoi(mp->fixed_id);
    }
    GUI_RemoveObj(obj);
    PAGE_ModelInit(-1); // must be -1 for devo10 to get back to correct page
}
static void fixedid_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_SetModal(1);
    if(Model.fixed_id == 0) {
        u32 id = PROTOCOL_CurrentID();
        if (id)
            sprintf(mp->fixed_id, "%d", (int)id);
        else
            mp->fixed_id[0] = 0;
    }
    PAGE_RemoveAllObjects();
    callback_result = 1;
    GUI_CreateKeyboard(&gui->keyboard, KEYBOARD_NUM, mp->fixed_id, 999999, fixedid_done_cb, &callback_result);
}

static void bind_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    if (PROTOCOL_AutoBindEnabled())
        PROTOCOL_Init(0);
    else
        PROTOCOL_Bind();
}

static void configure_bind_button()
{
    guiObject_t *obj = _get_obj(ITEM_PROTO, 1);
    if(obj)
        GUI_Redraw(obj);
    //GUI_SetHidden(mp->obj, PROTOCOL_AutoBindEnabled());
}

/* Text Select Callback */
static const char *type_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed = 0;
    Model.type = GUI_TextSelectHelper(Model.type, 0, 1, dir, 1, 1, &changed);
    GUI_TextSelectEnablePress((guiTextSelect_t *)obj, Model.type == 0);
    if (changed && Model.type != 0) {
        //Standard GUI is not supported
        Model.mixer_mode = MIXER_ADVANCED;
        guiObject_t *obj = _get_obj(ITEM_GUI, 0);
        if(obj)
            GUI_Redraw(obj);
    }

    switch (Model.type) {
        case 0: return _tr(HELI_LABEL);
        default: return _tr(PLANE_LABEL);
    }
}
void type_press_cb(guiObject_t *obj, void *data)
{
    (void)data;
    (void)obj;
    if(Model.type == 0) {
        PAGE_RemoveAllObjects();
        MODELPAGE_Config();
    }
}
static const char *numchanselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Model.num_channels = GUI_TextSelectHelper(Model.num_channels, 1, PROTOCOL_NumChannels(), dir, 1, 1, NULL);
    sprintf(mp->tmpstr, "%d", Model.num_channels);
    return mp->tmpstr;
}

static const char *powerselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    if(! PROTOCOL_HasPowerAmp(Model.protocol))
        return _tr("Default");
    Model.tx_power = GUI_TextSelectHelper(Model.tx_power, TXPOWER_100uW, TXPOWER_LAST-1, dir, 1, 1, NULL);
    mp->last_txpower = Model.tx_power;
    return RADIO_TX_POWER_VAL[Model.tx_power];
}

static const char *ppmin_select_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    u8 new_ppm = PPMin_Mode();
    new_ppm = GUI_TextSelectHelper(new_ppm, 0, 2, dir, 1, 1, &changed);

    if (obj) {
        if (changed) {
            if (! PPMin_Mode() && new_ppm) {
                //Start PPM-In
                PPMin_Start();
            } else if(! new_ppm) {
                //Stop PPM-In
                PPMin_Stop();
            }
        }
        GUI_TextSelectEnablePress((guiTextSelect_t *)obj, new_ppm);
        Model.num_ppmin = (Model.num_ppmin & 0x3f) | (new_ppm << 6);
    }
    if (new_ppm == 0) {
        return _tr("None");
    } else if (new_ppm == 1) {
        return _tr("Train");
    } else {
        return _tr("Input");
    }
}

void ppmin_press_cb(guiObject_t *obj, void *data)
{
    (void)data;
    (void)obj;
    PAGE_RemoveAllObjects();
    MODELTRAIN_Config();
    return;
}

static const char *protoselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    Model.protocol = GUI_TextSelectHelper(Model.protocol, PROTOCOL_NONE, PROTOCOL_COUNT-1, dir, 1, 1, &changed);
    if (changed) {
        PROTOCOL_DeInit();
        PROTOCOL_Load(1);
        Model.num_channels = PROTOCOL_DefaultNumChannels();
        if (! PROTOCOL_HasPowerAmp(Model.protocol))
            Model.tx_power = TXPOWER_150mW;
        else
            Model.tx_power = mp->last_txpower;
        memset(Model.proto_opts, 0, sizeof(Model.proto_opts));
        guiObject_t *obj = _get_obj(ITEM_NUMCHAN, 0);
        if (obj)
            GUI_Redraw(obj);
        obj = _get_obj(ITEM_TXPOWER, 0);
        if (obj)
            GUI_Redraw(obj);
        if (Model.mixer_mode == MIXER_STANDARD)
            STDMIXER_SetChannelOrderByProtocol();
        configure_bind_button();
    }
    GUI_TextSelectEnablePress((guiTextSelect_t *)obj, PROTOCOL_GetOptions() ? 1 : 0);
    if (Model.protocol == 0)
        return _tr("None");
    if(PROTOCOL_HasModule(Model.protocol))
        return ProtocolNames[Model.protocol];
    sprintf(mp->tmpstr, "*%s", ProtocolNames[Model.protocol]);
    return mp->tmpstr;
}
void proto_press_cb(guiObject_t *obj, void *data)
{
    (void)data;
    (void)obj;
    if(PROTOCOL_GetOptions()) {
        PAGE_RemoveAllObjects();
        MODELPROTO_Config();
    }
}
static const char *file_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    mp->file_state = GUI_TextSelectHelper(mp->file_state, 0, 3, dir, 1, 1, NULL);
    if (mp->file_state == 0)
        return _tr("Load...");
    else if (mp->file_state == 1)
        return _tr("Copy To...");
    else if (mp->file_state == 2)
        return _tr("Template..");
    else if (mp->file_state == 3)
        return _tr("Reset");
    else
        return "";
}

static void file_press_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    if (mp->file_state == 3) {
        CONFIG_ResetModel();
        CONFIG_SaveModelIfNeeded();
        GUI_RedrawAllObjects();
    } else {
        PAGE_SetModal(1);
        MODELPage_ShowLoadSave(mp->file_state, PAGE_ModelInit);
    }
}

static void changeicon_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    MODELPage_ShowLoadSave(3, PAGE_ModelInit);
}

static const char *mixermode_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    u8 changed = 0;
    int max_mode = Model.type ? 0 : 1; //Only allow Standard GUI for Helis
    Model.mixer_mode = GUI_TextSelectHelper(Model.mixer_mode, 0, max_mode, dir, 1, 1, &changed);
    if (changed && Model.mixer_mode == MIXER_STANDARD) {
        if (!STDMIXER_ValidateTraditionModel()) {
            Model.mixer_mode = MIXER_ADVANCED;
            PAGE_ShowInvalidStandardMixerDialog(obj);
        } else {
            STDMIXER_SetChannelOrderByProtocol();
        }
    }
    return STDMIXER_ModeName(Model.mixer_mode);
}

