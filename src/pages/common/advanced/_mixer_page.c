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

#define gui (&gui_objs.u.advmixer)
#define guim (&gui_objs.u.advmixcfg)
static struct mixer_page * const mp = &pagemem.u.mixer_page;
static u8 show_chantest;
static void templateselect_cb(guiObject_t *obj, const void *data);
static void limitselect_cb(guiObject_t *obj, const void *data);
static void virtname_cb(guiObject_t *obj, const void *data);
static const char *show_source(guiObject_t *obj, const void *data);

static void _show_title(int page);
static void _show_page();
static void _determine_save_in_live();

const char *MIXPAGE_ChannelNameCB(guiObject_t *obj, const void *data)
{
    (void)obj;
    return INPUT_SourceName(mp->tmpstr, (long)data + NUM_INPUTS + 1);
}

int _is_virt_cyclic(int ch)
{
    if (Model.type == MODELTYPE_HELI
        && (ch >= NUM_OUT_CHANNELS && ch < NUM_OUT_CHANNELS + 3))
    {
        int i;
        for (i = 0; i < Model.num_channels; i++) {
            if (Model.templates[i] == MIXERTEMPLATE_CYC1
                || Model.templates[i] == MIXERTEMPLATE_CYC2
                || Model.templates[i] == MIXERTEMPLATE_CYC3)
            {
                return 1;
            }
        }
    }
    return 0;
}

const char *MIXPAGE_ChanNameProtoCB(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 ch = (long)data;
    char tmp1[30];

    /* See if we need to name the cyclic virtual channels */
    if (_is_virt_cyclic(ch)) {
        switch(ch - NUM_OUT_CHANNELS) {
            case 0: snprintf(mp->tmpstr, sizeof(mp->tmpstr), "%s-%s", _tr("CYC"), _tr("AIL")); return mp->tmpstr;
            case 1: snprintf(mp->tmpstr, sizeof(mp->tmpstr), "%s-%s", _tr("CYC"), _tr("ELE")); return mp->tmpstr;
            case 2: snprintf(mp->tmpstr, sizeof(mp->tmpstr), "%s-%s", _tr("CYC"), _tr("COL")); return mp->tmpstr;
        }
    }
    if (ch < PROTO_MAP_LEN && ProtocolChannelMap[Model.protocol]) {
        INPUT_SourceNameAbbrevSwitch(tmp1, ProtocolChannelMap[Model.protocol][ch]);
        sprintf(mp->tmpstr, "%s%d-%s",
            (Model.limits[ch].flags & CH_REVERSE) ? "!" : "",
            (int)(ch + 1), tmp1);
    } else {
        INPUT_SourceName(tmp1, ch + NUM_INPUTS + 1);
        sprintf(mp->tmpstr, "%s%s",
                (ch < Model.num_channels && Model.limits[ch].flags & CH_REVERSE) ? "!" : "",
                tmp1);
    }
    return mp->tmpstr;
}

static const char *template_name_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 ch = (long)data;
    enum TemplateType template = MIXER_GetTemplate(ch);
    return MIXER_TemplateName(template);
}



void show_chantest_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    
    show_chantest = 1;
    PAGE_ChantestModal(PAGE_MixerInit, mp->top_channel);
}

void PAGE_ShowReorderList(u8 *list, u8 count, u8 selected, u8 max_allowed, const char *(*text_cb)(u8 idx), void(*return_page)(u8 *));
static const char *reorder_text_cb(u8 idx)
{
    long i = idx-1;
    return MIXPAGE_ChanNameProtoCB(NULL, (void *)i);
}

static void reorder_return_cb(u8 *list)
{
    if (list) {
        int i, j, k = 0;
        struct Mixer tmpmix[NUM_MIXERS];
        memset(tmpmix, 0, sizeof(tmpmix));
        for(j = 0; j < NUM_CHANNELS; j++) {
            for(i = 0; i <NUM_MIXERS; i++) {
                if(Model.mixers[i].src && Model.mixers[i].dest == list[j]-1) {
                    memcpy(&tmpmix[k], &Model.mixers[i], sizeof(struct Mixer));
                    tmpmix[k].dest = j;
                    k++;
                }
            }
        }
        memcpy(Model.mixers, tmpmix, sizeof(Model.mixers));
        struct Limit tmplimits[NUM_OUT_CHANNELS];
        u8 tmptemplates[NUM_CHANNELS];
        for(j = 0; j < NUM_CHANNELS; j++) {
            if(j < NUM_OUT_CHANNELS) {
               if(list[j]-1 < NUM_OUT_CHANNELS) {
                   tmplimits[j] = Model.limits[list[j]-1]; 
               } else {
                   MIXER_SetDefaultLimit(&tmplimits[j]);
               }
            }
            tmptemplates[j] = Model.templates[list[j]-1];
        }
        memcpy(Model.templates, tmptemplates, sizeof(Model.templates));
        memcpy(Model.limits, tmplimits, sizeof(Model.limits));
        MIXER_SetMixers(NULL, 0);
    }
    PAGE_MixerInit(mp->top_channel);
}

void reorder_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    PAGE_ShowReorderList(mp->list, NUM_CHANNELS, 0, 0, reorder_text_cb, reorder_return_cb);
}

void PAGE_MixerInit(int page)
{
    PAGE_SetModal(0);
    memset(mp, 0, sizeof(*mp));
    mp->top_channel = page;
    show_chantest = 0;
    _show_title(page);
    _show_page();
}

static const char *show_source(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 *source = (u8 *)data;
    return INPUT_SourceName(mp->tmpstr, *source);
}

static void _determine_save_in_live()
{
    if (mp->are_limits_changed) {
        mp->are_limits_changed = 0;
        MIXER_SetLimit(mp->channel, &mp->limit); // save limits' change in live
    }
}

void PAGE_MixerEvent()
{
    if (show_chantest) {
        PAGE_ChantestEvent();
        return;
    }
    // bug fix: when entering chantest modal page from the mixer page, the mp structure might be set to wrong value
    // and will clear all limit data in devo8, simply because all structures inside the pagemem are unions and share the same memory
    _determine_save_in_live();
    if (mp->cur_mixer && ! mp->edit.parent) {
        if (mp->cur_template == MIXERTEMPLATE_SIMPLE
            || mp->cur_template == MIXERTEMPLATE_EXPO_DR
            || mp->cur_template == MIXERTEMPLATE_COMPLEX)
        {
            if(MIXER_GetCachedInputs(mp->raw, CHAN_MAX_VALUE / 100)) { // +/-1%
                MIXPAGE_RedrawGraphs();
            }
        }
    }
}

void templateselect_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long idx = (long)data;
    u8 i;
    mp->cur_template = MIXER_GetTemplate(idx);
    PAGE_SetModal(1);
    MIXER_GetLimit(idx, &mp->limit);
    mp->channel = idx;
    mp->num_complex_mixers = 1;
    for(i = 0; i < sizeof(mp->mixer) / sizeof(struct Mixer); i++) {
        MIXER_InitMixer(mp->mixer + i, idx);
        mp->mixer[i].src = 0;
    }

    if (mp->cur_template != MIXERTEMPLATE_NONE) {
        mp->num_complex_mixers = MIXER_GetMixers(idx, mp->mixer, sizeof(mp->mixer) / sizeof(struct Mixer));
    }
    mp->link_curves = 0xff;
    if (mp->cur_template == MIXERTEMPLATE_EXPO_DR) {
        if (mp->num_complex_mixers > 1) {
            if (memcmp(&mp->mixer[1].curve, &mp->mixer[0].curve, sizeof(struct Curve)) != 0) {
                mp->link_curves &= ~0x01;
            }
            if (mp->num_complex_mixers > 2) {
                if (memcmp(&mp->mixer[2].curve, &mp->mixer[0].curve, sizeof(struct Curve)) != 0) {
                    mp->link_curves &= ~0x02;
                }
            }
        }
    }
    MIXPAGE_ChangeTemplate(1);
}

void limitselect_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long ch = (long)data;
    MIXER_GetLimit(ch, &mp->limit);
    mp->channel = ch;
    MIXPAGE_EditLimits();
}

static int callback_result;
static void _changename_done_cb(guiObject_t *obj, void *data)
{
    (void)data;
    GUI_RemoveObj(obj);
    PAGE_SetModal(0);
    if (callback_result) {
        int ch = callback_result - 1;
        strcpy(Model.virtname[ch], mp->tmpstr);
    }
}

void virtname_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int ch = (long)data - NUM_OUT_CHANNELS;
    PAGE_SetModal(1);
    if (Model.virtname[ch][0]) {
        strncpy(mp->tmpstr, Model.virtname[ch], sizeof(mp->tmpstr));
    } else {
        snprintf(mp->tmpstr, sizeof(mp->tmpstr), "Virt%d", ch+1); //Do not use _tr() here because the keyboard can't support it
    }
    callback_result = ch+1;
    GUI_CreateKeyboard(&gui->keyboard, KEYBOARD_ALPHA, mp->tmpstr, sizeof(Model.virtname[ch])-1, _changename_done_cb, &callback_result);
}

