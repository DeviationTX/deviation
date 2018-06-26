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
static struct modelpage_obj * const gui = &gui_objs.u.modelpage;
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
#if HAS_VIDEO
    ITEM_VIDEO,
#endif
    ITEM_PROTO,
    ITEM_FIXEDID,
    ITEM_NUMCHAN,
#if HAS_STANDARD_GUI
    ITEM_GUI,
#endif
    ITEM_LAST,
};

const char *show_text_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int width; int height;
    u16 txt_w, txt_h;
    tempstring_cpy((const char *)data);
    GUI_GetSize(obj, &width, &height);
    width -=2;
    while(1) {
        LCD_GetStringDimensions((const u8 *)tempstring, &txt_w, &txt_h);
        if (txt_w > width) {
            int len = strlen(tempstring);
            if (tempstring[len-1] == '.')
                len--;
            tempstring[len-3] = '.';
            tempstring[len-2] = '.';
            tempstring[len-1] = '\0';
        } else {
            break;
        }
    }
    return tempstring;
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
    Model.type = GUI_TextSelectHelper(Model.type, 0, MODELTYPE_LAST-1, dir, 1, 1, &changed);
    GUI_TextSelectEnablePress((guiTextSelect_t *)obj, Model.type == 0);
    if (changed && Model.type != 0) {
        //Standard GUI is not supported
        Model.mixer_mode = MIXER_ADVANCED;
#if HAS_STANDARD_GUI
        guiObject_t *obj = _get_obj(ITEM_GUI, 0);
        if(obj)
            GUI_Redraw(obj);
#endif
    }

    switch (Model.type) {
        case MODELTYPE_HELI: return _tr(HELI_LABEL);
        case MODELTYPE_PLANE: return _tr(PLANE_LABEL);
        case MODELTYPE_MULTI: return _tr(MULTI_LABEL);
        default: return 0; // supress warning.
    }
}
void type_press_cb(guiObject_t *obj, void *data)
{
    (void)data;
    (void)obj;
    if(Model.type == 0) {
        PAGE_PushByID(PAGEID_TYPECFG, 0);
    }
}
static const char *numchanselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Model.num_channels = GUI_TextSelectHelper(Model.num_channels, 1, PROTOCOL_NumChannels(), dir, 1, 1, NULL);
    sprintf(tempstring, "%d", Model.num_channels);
    return tempstring;
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
    new_ppm = GUI_TextSelectHelper(new_ppm, 0, 3, dir, 1, 1, &changed);

    if (obj) {
        if (changed) {
            if (! PPMin_Mode() && new_ppm) {
                //Start PPM-In
                PPMin_Start();
            } else if(! new_ppm) {
                //Stop PPM-In
                PPMin_Stop();
            }
            switch (new_ppm) {
                case PPM_IN_TRAIN1:
                    memset(Model.ppm_map, -1, sizeof(Model.ppm_map));
                    for(int i = 0; i < Model.num_channels; i++)
                        Model.ppm_map[i] = i;
                    break;
                case PPM_IN_TRAIN2:
                    memset(Model.ppm_map, -1, sizeof(Model.ppm_map));
                    for(int i = 0; i < 4; i++)
                        Model.ppm_map[i] = i+1;
                    break;
            }
        }
        GUI_TextSelectEnablePress((guiTextSelect_t *)obj, new_ppm);
        Model.num_ppmin = (Model.num_ppmin & 0x3f) | (new_ppm << 6);
    }
    if (new_ppm == 0) {
        return _tr("None");
    } else if (new_ppm == 1) {
        return _tr("Channel");
    } else if (new_ppm == 2) {
        return _tr("Stick");
    } else {
        return _tr("Extend");
    }
}

void ppmin_press_cb(guiObject_t *obj, void *data)
{
    (void)data;
    (void)obj;
    PAGE_PushByID(PAGEID_TRAINCFG, 0);
    return;
}

static const char *protoselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    enum Protocols new_protocol;
    new_protocol = GUI_TextSelectHelper(Model.protocol, PROTOCOL_NONE, PROTOCOL_COUNT-1, dir, 1, 1, &changed);
    if (changed) {
        const u8 *oldmap = ProtocolChannelMap[Model.protocol];
    	// DeInit() the old protocol (Model.protocol unchanged)
        PROTOCOL_DeInit();
        // Load() the new protocol
        Model.protocol = new_protocol;
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
        else
            RemapChannelsForProtocol(oldmap);
        configure_bind_button();
    }
    GUI_TextSelectEnablePress((guiTextSelect_t *)obj, PROTOCOL_GetOptions() ? 1 : 0);
    if (Model.protocol == 0)
        return _tr("None");
    if(PROTOCOL_HasModule(Model.protocol))
        return ProtocolNames[Model.protocol];
    sprintf(tempstring, "*%s", ProtocolNames[Model.protocol]);
    return tempstring;
}
void proto_press_cb(guiObject_t *obj, void *data)
{
    (void)data;
    (void)obj;
    if(PROTOCOL_GetOptions()) {
        PAGE_PushByID(PAGEID_PROTOCFG, 0);
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
        FullRedraw = REDRAW_EVERYTHING;
    } else {
        PAGE_PushByID(PAGEID_LOADSAVE, mp->file_state);
    }
}

static void changeicon_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_PushByID(PAGEID_LOADSAVE, LOAD_ICON);
}

#if HAS_VIDEO
static void video_settings_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_PushByID(PAGEID_VIDEOCFG, 0);
}
#endif //HAS_VIDEO

#if HAS_STANDARD_GUI
static const char *mixermode_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    u8 changed = 0;
    int max_mode = Model.type ? 0 : 1; //Only allow Standard GUI for Helis
    Model.mixer_mode = GUI_TextSelectHelper(Model.mixer_mode-1, 0, max_mode, dir, 1, 1, &changed) + 1;
    if (changed && Model.mixer_mode == MIXER_STANDARD) {
        if (!STDMIXER_ValidateTraditionModel()) {
            Model.mixer_mode = MIXER_ADVANCED;
            PAGE_ShowInvalidStandardMixerDialog(obj);
        } else {
            STDMIXER_SetChannelOrderByProtocol();
            STDMIXER_SaveSwitches();
        }
    }
    return STDMIXER_ModeName(Model.mixer_mode);
}
#endif //HAS_STANDARD_GUI
