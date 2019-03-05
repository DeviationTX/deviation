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

static struct advmixer_obj  * const gui  = &gui_objs.u.advmixer;
static struct advmixcfg_obj * const guim = &gui_objs.u.advmixcfg;
static struct mixer_page    * const mp   = &pagemem.u.mixer_page;
static u16 current_selected = 0;

static void templateselect_cb(guiObject_t *obj, const void *data);
static void limitselect_cb(guiObject_t *obj, const void *data);
static void virtname_cb(guiObject_t *obj, const void *data);
static const char *show_source(guiObject_t *obj, const void *data);

static void _show_title();
static void _show_page();

const char *MIXPAGE_ChannelNameCB(guiObject_t *obj, const void *data)
{
    (void)obj;
    return INPUT_SourceName(tempstring, (long)data + NUM_INPUTS + 1);
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
    u8 proto_map_length = PROTO_MAP_LEN;
    char tmp1[30];

    /* See if we need to name the cyclic virtual channels */
    if (_is_virt_cyclic(ch)) {
        switch(ch - NUM_OUT_CHANNELS) {
            case 0: snprintf(tempstring, sizeof(tempstring), "%s-%s", _tr("CYC"), _tr("AIL")); return tempstring;
            case 1: snprintf(tempstring, sizeof(tempstring), "%s-%s", _tr("CYC"), _tr("ELE")); return tempstring;
            case 2: snprintf(tempstring, sizeof(tempstring), "%s-%s", _tr("CYC"), _tr("COL")); return tempstring;
        }
    }
    #if defined(HAS_SWITCHES_NOSTOCK) && HAS_SWITCHES_NOSTOCK
    #define SWITCH_NOSTOCK ((1 << INP_HOLD0) | (1 << INP_HOLD1) | \
                            (1 << INP_FMOD0) | (1 << INP_FMOD1))
    if ((Transmitter.ignore_src & SWITCH_NOSTOCK) == SWITCH_NOSTOCK)
        proto_map_length = PROTO_MAP_LEN - 1;
    #endif //HAS_SWITCHES_NOSTOCK
    if (ch < proto_map_length && CurrentProtocolChannelMap) {
        INPUT_SourceNameAbbrevSwitch(tmp1, CurrentProtocolChannelMap[ch]);
        sprintf(tempstring, "%s%d-%s",
            (Model.limits[ch].flags & CH_REVERSE) ? "!" : "",
            (int)(ch + 1), tmp1);
    } else {
        INPUT_SourceName(tmp1, ch + NUM_INPUTS + 1);
        sprintf(tempstring, "%s%s",
                (ch < Model.num_channels && Model.limits[ch].flags & CH_REVERSE) ? "!" : "",
                tmp1);
    }
    return tempstring;
}

static const char *template_name_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 ch = (long)data;
    enum TemplateType template = MIXER_GetTemplate(ch);
    return _tr(MIXER_TemplateName(template));
}

void show_chantest_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    
    PAGE_PushByID(PAGEID_CHANMON, 0);
}

static const char *reorder_text_cb(u8 idx)
{
    long i = idx-1;
    return MIXPAGE_ChanNameProtoCB(NULL, (void *)i);
}

static void reorder_mixers_by_list(u8 *list)
{
    u8 reorder[NUM_CHANNELS];
    memset(reorder, 0xff, sizeof(reorder));
    for (int newdest = 0; newdest < NUM_CHANNELS; newdest++) {
        int olddest = list[newdest];
        if (olddest) {
            reorder[olddest-1] = newdest;
        }
    }
    for (int i = 0; i < NUM_MIXERS; i++) {
        int dest = Model.mixers[i].dest;
        if (!Model.mixers[i].src)
            break;
        if (reorder[dest] != 0xff) {
            Model.mixers[i].dest = reorder[dest];
        }
    }
}

static void reorder_limits_by_list(u8 *list)
{
    unsigned j;
    struct Limit tmplimits[NUM_OUT_CHANNELS];
    u8 tmptemplates[NUM_CHANNELS];
    for (j = 0; j < NUM_CHANNELS; j++) {
        if (j < NUM_OUT_CHANNELS) {
           if (list[j]-1 < NUM_OUT_CHANNELS) {
               tmplimits[j] = Model.limits[list[j]-1];
           } else {
               MIXER_SetDefaultLimit(&tmplimits[j]);
           }
        }
        tmptemplates[j] = Model.templates[list[j]-1];
    }
    memcpy(Model.templates, tmptemplates, sizeof(Model.templates));
    memcpy(Model.limits, tmplimits, sizeof(Model.limits));
}

static void reorder_return_cb(u8 *list)
{
    if (!list)
        return;
    reorder_mixers_by_list(list);
    reorder_limits_by_list(list);
    MIXER_SetMixers(NULL, 0);
}

static void reorder_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    PAGE_ShowReorderList(mp->list, NUM_CHANNELS, 0, 0, reorder_text_cb, reorder_return_cb);
}

void PAGE_MixerInit(int page)
{
    (void)page;
    memset(mp, 0, sizeof(*mp));
    _show_title();
    _show_page();
}

static const char *show_source(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 *source = (u8 *)data;
    return INPUT_SourceName(tempstring, *source);
}

void templateselect_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long idx = (long)data;
    u8 i;
    mp->cur_template = MIXER_GetTemplate(idx);
    mp->limit = MIXER_GetLimit(idx);
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
    PAGE_PushByID(PAGEID_MIXTEMPL, 0);
}

void limitselect_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long ch = (long)data;
    PAGE_PushByID(PAGEID_EDITLIMIT, ch);
}

static int callback_result;
static void _changename_done_cb(guiObject_t *obj, void *data)
{
    (void)data;
    GUI_RemoveObj(obj);
    PAGE_SetModal(0);
    if (callback_result) {
        int ch = callback_result - 1;
        strlcpy(Model.virtname[ch], tempstring, sizeof(Model.virtname[ch]));
    }
}

void virtname_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int ch = (long)data - NUM_OUT_CHANNELS;
    PAGE_SetModal(1);
    if (Model.virtname[ch][0]) {
        tempstring_cpy(Model.virtname[ch]);
    } else {
        snprintf(tempstring, sizeof(tempstring), "Virt%d", ch+1); //Do not use _tr() here because the keyboard can't support it
    }
    callback_result = ch+1;
    GUI_CreateKeyboard(&gui->keyboard, KEYBOARD_ALPHA, tempstring, sizeof(Model.virtname[ch])-1, _changename_done_cb, &callback_result);
}

#define TESTNAME pages_advanced_mixer
#include <tests.h>
