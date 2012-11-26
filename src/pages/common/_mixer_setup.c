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

static struct mixer_page * const mp = &pagemem.u.mixer_page;

static const char *templatetype_cb(guiObject_t *obj, int value, void *data);
static void sync_mixers();
static const char *set_number100_cb(guiObject_t *obj, int dir, void *data);
static s16 eval_mixer_cb(s16 xval, void * data);
static s16 eval_chan_cb(void * data);
static u8 curpos_cb(s16 *x, s16 *y, u8 pos, void *data);
static void toggle_link_cb(guiObject_t *obj, const void *data);
static const char *show_trim_cb(guiObject_t *obj, const void *data);
static void toggle_trim_cb(guiObject_t *obj, const void *data);

void PAGE_ShowReorderList(u8 *list, u8 count, u8 selected, u8 max_allowed, const char *(*text_cb)(u8 idx), void(*return_page)(u8 *));
static void reorder_cb(guiObject_t *obj, void *data);

static void _show_titlerow();
static void show_none();
static void _show_simple();
static void _show_expo_dr();
static void _show_complex();
static void redraw_graphs();
static void _update_rate_widgets(u8 idx);

void MIXPAGE_ChangeTemplate(int show_header)
{
    if (mp->cur_template != MIXERTEMPLATE_COMPLEX
        || mp->cur_mixer < mp->mixer
        || mp->cur_mixer > mp->mixer + mp->num_mixers)
    {
        mp->cur_mixer = mp->mixer;
    }
    if (Model.type == MODELTYPE_PLANE && mp->cur_template > MIXERTEMPLATE_MAX_PLANE) {
        mp->cur_template = MIXERTEMPLATE_NONE;
    }
    sync_mixers();
    if (show_header) {
        PAGE_RemoveAllObjects();
        _show_titlerow();
    } else {
        GUI_RemoveHierObjects(mp->firstObj); 
    }
    memset(mp->expoObj, 0, sizeof(mp->expoObj));
    memset(mp->graphs, 0, sizeof(mp->graphs));
    mp->trimObj = NULL;
    switch(mp->cur_template)  {
    case MIXERTEMPLATE_NONE:
    case MIXERTEMPLATE_CYC1:
    case MIXERTEMPLATE_CYC2:
    case MIXERTEMPLATE_CYC3:
        show_none();
        break;
    case MIXERTEMPLATE_SIMPLE:
        _show_simple();
        break;
    case MIXERTEMPLATE_EXPO_DR:
        _show_expo_dr();
        break;
    case MIXERTEMPLATE_COMPLEX:
        _show_complex();
        break;
    }
}


static const char *templatetype_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed;
    mp->cur_template = GUI_TextSelectHelper(mp->cur_template, 0, Model.type == MODELTYPE_HELI ? MIXERTEMPLATE_MAX_HELI : MIXERTEMPLATE_MAX_PLANE, dir, 1, 1, &changed);
    if (changed) {
        MIXPAGE_ChangeTemplate(0);
        return "";
    }
    return MIXER_TemplateName(mp->cur_template);
}

static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data);
static void sourceselect_cb(guiObject_t *obj, void *data);
static void curveselect_cb(guiObject_t *obj, void *data);
static const char *set_source_cb(guiObject_t *obj, int dir, void *data);
static const char *set_drsource_cb(guiObject_t *obj, int dir, void *data);
static const char *set_mux_cb(guiObject_t *obj, int dir, void *data);
static const char *set_nummixers_cb(guiObject_t *obj, int dir, void *data);
static const char *set_mixernum_cb(guiObject_t *obj, int dir, void *data);
static void okcancel_cb(guiObject_t *obj, const void *data);
static u8 touch_cb(s16 x, s16 y, void *data);

static void show_none()
{
    mp->firstObj = NULL;   
    //Row 0
}

static const char *show_trim_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return (long)mp->cur_mixer->apply_trim ? _tr("Trim") : _tr("No Trim");
}

static void toggle_trim_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    mp->cur_mixer->apply_trim ^= 0x01;
}

void toggle_link_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    if(data) {
        mp->link_curves ^= 0x02;
        if (mp->link_curves & 0x02) { //Redraw graphs when re-linking
            sync_mixers();
            redraw_graphs();
        }
    } else {
        mp->link_curves ^= 0x01;
        if (mp->link_curves & 0x01) { //Redraw graphs when re-linking
            sync_mixers();
            redraw_graphs();
        }
    }
    _update_rate_widgets(data ? 1 : 0);
}

static const char *show_rate_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return (long)data == 0 ? _tr("Mid-Rate") : _tr("Low-Rate");
}

s16 eval_mixer_cb(s16 xval, void * data)
{
    struct Mixer *mix = (struct Mixer *)data;
    if (MIXER_SRC_IS_INV(mix->src))
        xval = -xval;
    s16 yval = CURVE_Evaluate(xval, &mix->curve);
    yval = yval * mix->scalar / 100 + PCT_TO_RANGE(mix->offset);

    /* Min/Max is a servo limit, shouldn't be shown here
    if(mix->dest < NUM_OUT_CHANNELS) {
        if (yval > PCT_TO_RANGE(mp->limit.max))
            yval = PCT_TO_RANGE(mp->limit.max);
        else if (yval < PCT_TO_RANGE(mp->limit.min))
            yval = PCT_TO_RANGE(mp->limit.min);
    }
    */

    if (yval > CHAN_MAX_VALUE)
        yval = CHAN_MAX_VALUE;
    else if (yval <CHAN_MIN_VALUE)
        yval = CHAN_MIN_VALUE;
    //Don't showchannel-reverse on the graph (but do show input reverse)
    //if (mp->limit.flags & CH_REVERSE)
    //    yval = -yval;
    return yval;
}
s16 eval_chan_cb(void * data)
{
    (void)data;
    int i;
    struct Mixer *mix = MIXER_GetAllMixers();
    for (i = 0; i < NUM_MIXERS; i++) {
        if(MIXER_SRC(mix->src) != 0 && mix->dest != mp->cur_mixer->dest)
            MIXER_ApplyMixer(mix, mp->raw);
    }
    for (i = 0; i < mp->num_mixers; i++)
        MIXER_ApplyMixer(&mp->mixer[i], mp->raw);
    s16 value = MIXER_ApplyLimits(mp->cur_mixer->dest, &mp->limit, mp->raw, NULL, APPLY_ALL);
    if (value > CHAN_MAX_VALUE)
        return CHAN_MAX_VALUE;
    if (value < CHAN_MIN_VALUE)
        return CHAN_MIN_VALUE;
    return value;
}

u8 curpos_cb(s16 *x, s16 *y, u8 pos, void *data)
{
    if (pos != 0)
        return 0;
    *x = mp->raw[MIXER_SRC(mp->cur_mixer->src)];
    if (*x > CHAN_MAX_VALUE)
        *x = CHAN_MAX_VALUE;
    else if (*x  < CHAN_MIN_VALUE)
        *x = CHAN_MIN_VALUE;
    *y = eval_mixer_cb(*x, data);
    return 1;
}

const char *PAGEMIXER_SetNumberCB(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    s8 *value = (s8 *)data;
    *value = GUI_TextSelectHelper(*value, -100, 100, dir, 1, LONG_PRESS_STEP, NULL);
    sprintf(mp->tmpstr, "%d", *value);
    return mp->tmpstr;
}

void sync_mixers()
{
    switch(mp->cur_template) {
    case MIXERTEMPLATE_NONE:
    case MIXERTEMPLATE_CYC1:
    case MIXERTEMPLATE_CYC2:
    case MIXERTEMPLATE_CYC3:
        mp->num_mixers = 0;
        break;
    case MIXERTEMPLATE_SIMPLE:
        mp->mixer[0].sw = 0;
        mp->mixer[0].mux = MUX_REPLACE;
        mp->num_mixers = 1;
        break;
    case MIXERTEMPLATE_COMPLEX:
        mp->num_mixers = mp->num_complex_mixers;
        break;
    case MIXERTEMPLATE_EXPO_DR:
        mp->num_mixers = 1;
        if (MIXER_SRC(mp->mixer[1].sw)) {
            mp->num_mixers++;
            mp->mixer[1].src    = mp->mixer[0].src;
            mp->mixer[1].dest   = mp->mixer[0].dest;
            mp->mixer[1].mux    = MUX_REPLACE;
            mp->mixer[1].offset = 0;
            mp->mixer[1].apply_trim = mp->mixer[0].apply_trim;
            if (mp->link_curves & 0x01)
                mp->mixer[1].curve = mp->mixer[0].curve;
        } else {
            mp->mixer[1].src = 0;
        }
        if (MIXER_SRC(mp->mixer[2].sw)) {
            mp->num_mixers++;
            mp->mixer[2].src    = mp->mixer[0].src;
            mp->mixer[2].dest   = mp->mixer[0].dest;
            mp->mixer[2].mux    = MUX_REPLACE;
            mp->mixer[2].offset = 0;
            mp->mixer[2].apply_trim = mp->mixer[0].apply_trim;
            if (mp->link_curves & 0x02)
                mp->mixer[2].curve = mp->mixer[0].curve;
        } else {
            mp->mixer[2].src = 0;
        }
        mp->mixer[0].mux = MUX_REPLACE;
        mp->mixer[0].offset = 0;
        mp->mixer[0].sw = 0;
        break;
    }
}

const char *set_number100_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 changed;
    s8 *value = (s8 *)data;
    if (!GUI_IsTextSelectEnabled(obj) ) {
        sprintf(mp->tmpstr, "%d", *value);
        return mp->tmpstr;
    }
    s8 min = -100; //(value == &mp->limit.max) ? mp->limit.min : -100;
    s8 max = 100; //(value == &mp->limit.min) ? mp->limit.max : 100;
    *value = GUI_TextSelectHelper(*value, min, max, dir, 1, 5, &changed);
    sprintf(mp->tmpstr, "%d", *value);
    if (changed) {
        sync_mixers();
        redraw_graphs();
    }
    return mp->tmpstr;
}

const char *set_mux_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed;
    mp->cur_mixer->mux = GUI_TextSelectHelper(mp->cur_mixer->mux, MUX_REPLACE, MUX_LAST-1, dir, 1, 1, &changed);
    if (changed) {
        redraw_graphs();
        sync_mixers();
    }
    switch(mp->cur_mixer->mux) {
        case MUX_REPLACE:  return _tr("replace");
        case MUX_MULTIPLY: return _tr("mult");
        case MUX_ADD:      return _tr("add");
        case MUX_MAX:      return _tr("max");
        case MUX_MIN:      return _tr("min");
        case MUX_LAST: break;
    }
    return "";
}

const char *set_nummixers_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed;
    mp->num_mixers = GUI_TextSelectHelper(
                     mp->num_mixers,
                     1 + (mp->cur_mixer - mp->mixer),
                     NUM_COMPLEX_MIXERS,
                     dir, 1, 1, &changed);
    if (changed) {
        mp->num_complex_mixers = mp->num_mixers;
        redraw_graphs();
        sync_mixers();
    }
    sprintf(mp->tmpstr, "%d", mp->num_mixers);
    return mp->tmpstr;
}

const char *set_mixernum_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 cur = (mp->cur_mixer - mp->mixer) + 1;
    u8 changed;
    cur = GUI_TextSelectHelper(cur, 1, mp->num_mixers, dir, 1, 1, &changed);
    if (changed) {
        mp->cur_mixer = mp->mixer + (cur - 1);
        _show_complex();
    }
    sprintf(mp->tmpstr, "%d", cur);
    return mp->tmpstr;
}

const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    u8 is_neg = MIXER_SRC_IS_INV(*source);
    u8 changed;
    *source = GUI_TextSelectHelper(MIXER_SRC(*source), 0, NUM_SOURCES, dir, 1, 1, &changed);
    MIXER_SET_SRC_INV(*source, is_neg);
    if (changed) {
        if(mp->trimObj) {
            if (MIXER_SourceHasTrim(MIXER_SRC(mp->mixer[0].src)))
                GUI_SetHidden(mp->trimObj, 0);
            else
                GUI_SetHidden(mp->trimObj, 1);
        }
        sync_mixers();
        redraw_graphs();
    }
    GUI_TextSelectEnablePress(obj, MIXER_SRC(*source));
    return INPUT_SourceName(mp->tmpstr, *source);
}

const char *set_drsource_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    u8 is_neg = MIXER_SRC_IS_INV(*source);
    u8 changed;
    u8 oldsrc = *source;
    *source = GUI_TextSelectHelper(MIXER_SRC(*source), 0, NUM_SOURCES, dir, 1, 1, &changed);
    MIXER_SET_SRC_INV(*source, is_neg);
    if (changed) {
        sync_mixers();
        if ((!! MIXER_SRC(oldsrc)) ^ (!! MIXER_SRC(*source))) {
            if(data == &mp->mixer[1].sw)
                _update_rate_widgets(0);
            else if(data == &mp->mixer[2].sw)
                _update_rate_widgets(1);
        } else {    
            redraw_graphs();
        }
    }
    GUI_TextSelectEnablePress(obj, MIXER_SRC(*source));
    return INPUT_SourceName(mp->tmpstr, *source);
}

static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    if (!GUI_IsTextSelectEnabled(obj)) {
        strcpy(mp->tmpstr, _tr("Linked"));
        return mp->tmpstr;
    }
    u8 changed;
    struct Mixer *mix = (struct Mixer *)data;
    mix->curve.type = GUI_TextSelectHelper(mix->curve.type, 0, CURVE_MAX, dir, 1, 1, &changed);
    if (changed) {
        sync_mixers();
        redraw_graphs();
    }
    GUI_TextSelectEnablePress(obj, mix->curve.type >= CURVE_EXPO);
    return CURVE_GetName(&mix->curve);
}

void sourceselect_cb(guiObject_t *obj, void *data)
{
    u8 *source = (u8 *)data;
    if (MIXER_SRC(*source)) {
        MIXER_SET_SRC_INV(*source, ! MIXER_SRC_IS_INV(*source));
        GUI_Redraw(obj);
        redraw_graphs();
    }
}

void graph_cb()
{
    MIXPAGE_ChangeTemplate(1);
}

void curveselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if (obj && !GUI_IsTextSelectEnabled(obj)) {
        return;
    }
    struct Mixer *mix = (struct Mixer *)data;
    int idx = (mix == &mp->mixer[1]) ? 1 : (mix == &mp->mixer[2]) ? 2 : 0;
    if (mix->curve.type >= CURVE_EXPO
        && (mp->cur_template != MIXERTEMPLATE_EXPO_DR || mix == 0 || ! (mp->link_curves & idx))) {
        //Do not allow entering a linked graph
        memset(mp->graphs, 0, sizeof(mp->graphs));
        MIXPAGE_EditCurves(&mix->curve, graph_cb);
    }
}

static const char *reorder_text_cb(u8 idx)
{
    if(! idx)
        return "";
    if(idx == 255)
        return _tr("New");
    sprintf(mp->tmpstr, "%s %d", _tr("Mixer"), idx);
    return mp->tmpstr;
}
static void reorder_return_cb(u8 *list)
{
    int i;
    if (list) {
        struct Mixer tmpmix[NUM_COMPLEX_MIXERS];
        int new_cur_mixer = mp->cur_mixer - mp->mixer;
        for(i = 0; i < NUM_COMPLEX_MIXERS; i++) {
            if(list[i] == 0)
                break;
            if(list[i] == 255) {
                memset(&tmpmix[i], 0, sizeof(struct Mixer));
            } else {
                tmpmix[i] = mp->mixer[list[i]-1];
                if(mp->cur_mixer - mp->mixer == list[i]-1)
                    new_cur_mixer = i;
            }
        }
        mp->cur_mixer = mp->mixer + new_cur_mixer;
        if (mp->cur_mixer - mp->mixer >= i)
            mp->cur_mixer = mp->mixer + i - 1;
        memcpy(mp->mixer, tmpmix, sizeof(mp->mixer));
        mp->num_complex_mixers = i;
    }
    MIXPAGE_ChangeTemplate(1);
}
void reorder_cb(guiObject_t *obj, void *data)
{
    (void)data;
    (void)obj;
    PAGE_ShowReorderList(mp->list, mp->num_mixers,mp->cur_mixer - mp->mixer,
                         NUM_COMPLEX_MIXERS,
                         reorder_text_cb, reorder_return_cb);
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    if (data) {
        //Save mixer here
        MIXER_SetLimit(mp->channel, &mp->limit);
        MIXER_SetTemplate(mp->channel, mp->cur_template);
        MIXER_SetMixers(mp->mixer, mp->num_mixers);
    }
    GUI_RemoveAllObjects();
    memset(mp->graphs, 0, sizeof(mp->graphs));
    PAGE_MixerInit(mp->top_channel);
}

static u8 touch_cb(s16 x, s16 y, void *data)
{
    (void)x;
    (void)y;
    curveselect_cb(NULL, data);
    return 1;
}
void redraw_graphs()
{
    switch(mp->cur_template) {
    case MIXERTEMPLATE_EXPO_DR:
        if (mp->graphs[1] != NULL)
            GUI_Redraw(mp->graphs[1]);
        if (mp->graphs[2] != NULL)
            GUI_Redraw(mp->graphs[2]);
    case MIXERTEMPLATE_COMPLEX:
    case MIXERTEMPLATE_SIMPLE:
        if (mp->graphs[0] != NULL)
            GUI_Redraw(mp->graphs[0]);
        break;
    case MIXERTEMPLATE_NONE: break;
    case MIXERTEMPLATE_CYC1: break;
    case MIXERTEMPLATE_CYC2: break;
    case MIXERTEMPLATE_CYC3: break;
    }
}
