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

static struct mixer_page * const mp = &pagemem.u.mixer_page;

static const char *templatetype_cb(guiObject_t *obj, int value, void *data);
static void sync_mixers();
static const char *set_number100_cb(guiObject_t *obj, int dir, void *data);
static s16 eval_mixer_cb(s16 xval, void * data);
static s16 eval_chan_cb(void * data);
static u8 curpos_cb(s16 *x, s16 *y, u8 pos, void *data);
static void toggle_link_cb(guiObject_t *obj, void *data);

static void show_titlerow();
static void show_none();
static void show_simple();
static void show_expo_dr();
static void show_complex();
static void redraw_graphs();


void MIXPAGE_ChangeTemplate(int show_header)
{
    mp->cur_mixer = mp->mixer;
    sync_mixers();
    if (show_header) {
        PAGE_RemoveAllObjects();
        show_titlerow();
    } else {
        GUI_RemoveHierObjects(mp->firstObj); 
    }
    memset(mp->expoObj, 0, sizeof(mp->expoObj));
    memset(mp->graphs, 0, sizeof(mp->graphs));
    switch(mp->cur_template)  {
    case MIXERTEMPLATE_NONE:
        show_none();
        break;
    case MIXERTEMPLATE_SIMPLE:
        show_simple();
        break;
    case MIXERTEMPLATE_EXPO_DR:
        show_expo_dr();
        break;
    case MIXERTEMPLATE_COMPLEX:
        show_complex();
        break;
    }
}


static const char *templatetype_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed;
    mp->cur_template = GUI_TextSelectHelper(mp->cur_template, 0, MIXERTEMPLATE_MAX, dir, 1, 1, &changed);
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
static void okcancel_cb(guiObject_t *obj, void *data);
static u8 touch_cb(s16 x, s16 y, void *data);

#define COL1_TEXT   8
#define COL1_VALUE  56
#define COL2_TEXT  168
#define COL2_VALUE 216
static void show_titlerow()
{
    GUI_CreateLabel(4, 10, MIXPAGE_ChanNameProtoCB, TITLE_FONT, (void *)((long)mp->cur_mixer->dest));
    GUI_CreateTextSelect(COL1_VALUE, 8, TEXTSELECT_96, 0x0000, NULL, templatetype_cb, (void *)((long)mp->channel));
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);
}

static void show_none()
{
    mp->firstObj = NULL;   
    //Row 0
}

static void show_simple()
{
    //Row 1
    mp->firstObj = GUI_CreateLabel(COL1_TEXT, 40, NULL, DEFAULT_FONT, "Src:");
    GUI_CreateTextSelect(COL1_VALUE, 40, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->mixer[0].src);
    GUI_CreateLabel(COL2_TEXT, 40, NULL, DEFAULT_FONT, "Curve:");
    GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[0]);
    //Row 2
    mp->graphs[0] = GUI_CreateXYGraph(112, 64, 120, 120,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[0]);
    //Row 4
    GUI_CreateLabel(COL1_TEXT, 192, NULL, DEFAULT_FONT, "Scale:");
    GUI_CreateTextSelect(COL1_VALUE, 192, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[0].scalar);
    GUI_CreateLabel(COL2_TEXT, 192, NULL, DEFAULT_FONT, "Offset:");
    GUI_CreateTextSelect(COL2_VALUE, 192, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[0].offset);
    //Row 5
    GUI_CreateLabel(COL1_TEXT, 216, NULL, DEFAULT_FONT, "Min:");
    GUI_CreateTextSelect(COL1_VALUE, 216, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.min);
    GUI_CreateLabel(COL2_TEXT, 216, NULL, DEFAULT_FONT, "Max:");
    GUI_CreateTextSelect(COL2_VALUE, 216, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.max);
}

static void update_rate_widgets(u8 idx)
{
    u8 mix = idx + 1;
    idx *=4;
    if (MIXER_SRC(mp->mixer[mix].sw)) {
        GUI_SetHidden(mp->expoObj[idx], 0);
        if(mp->link_curves & mix) {
            GUI_SetHidden(mp->expoObj[idx+1], 0);
            GUI_SetHidden(mp->expoObj[idx+2], 1);
        } else {
            GUI_SetHidden(mp->expoObj[idx+1], 1);
            GUI_SetHidden(mp->expoObj[idx+2], 0);
        }
        GUI_SetHidden(mp->expoObj[idx+3], 0);
        GUI_SetHidden(mp->graphs[mix], 0);
    } else {
        GUI_SetHidden(mp->expoObj[idx], 1);
        GUI_SetHidden(mp->expoObj[idx+1], 1);
        GUI_SetHidden(mp->expoObj[idx+2], 1);
        GUI_SetHidden(mp->expoObj[idx+3], 1);
        GUI_SetHidden(mp->graphs[mix], 1);
    }
}

void toggle_link_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if(data)
       mp->link_curves ^= 0x02;
    else
       mp->link_curves ^= 0x01;
    update_rate_widgets(data ? 1 : 0);
}

static const char *show_rate_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    return (long)data == 0 ? "Mid-Rate" : "Low-Rate";
}

static void show_expo_dr()
{
    sync_mixers();
    //Row 1
    mp->firstObj = GUI_CreateLabel(40, 34, NULL, DEFAULT_FONT, "Src");
    GUI_CreateLabel(132, 34, NULL, DEFAULT_FONT, "Switch1");
    GUI_CreateLabel(236, 34, NULL, DEFAULT_FONT, "Switch2");
    //Row 2
    GUI_CreateTextSelect(COL1_TEXT, 48, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->mixer[0].src);
    GUI_CreateTextSelect(112, 48, TEXTSELECT_96, 0x0000, sourceselect_cb, set_drsource_cb, &mp->mixer[1].sw);
    GUI_CreateTextSelect(216, 48, TEXTSELECT_96, 0x0000, sourceselect_cb, set_drsource_cb, &mp->mixer[2].sw);
    //Row 3
    GUI_CreateLabel(24, 74, NULL, DEFAULT_FONT, "High-Rate");
    mp->expoObj[0] = GUI_CreateButton(112, 72, BUTTON_96x16, show_rate_cb, 0x0000, toggle_link_cb, (void *)0);
    mp->expoObj[4] = GUI_CreateButton(216, 72, BUTTON_96x16, show_rate_cb, 0x0000, toggle_link_cb, (void *)1);
    //Row 4
    GUI_CreateTextSelect(COL1_TEXT, 96, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[0]);
    //The following 2 items are mutex.  One is always hidden
    mp->expoObj[1] = GUI_CreateLabel(140, 98, NULL, DEFAULT_FONT, "Linked");
    mp->expoObj[2] = GUI_CreateTextSelect(112, 96, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[1]);
    //The following 2 items are mutex.  One is always hidden
    mp->expoObj[5] = GUI_CreateLabel(244, 98, NULL, DEFAULT_FONT, "Linked");
    mp->expoObj[6] = GUI_CreateTextSelect(216, 96, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[2]);
    //Row 5
    GUI_CreateLabel(COL1_TEXT, 122, NULL, DEFAULT_FONT, "Scale:");
    GUI_CreateTextSelect(40, 120, TEXTSELECT_64, 0x0000, NULL, set_number100_cb, &mp->mixer[0].scalar);
    mp->expoObj[3] = GUI_CreateTextSelect(112, 120, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[1].scalar);
    mp->expoObj[7] = GUI_CreateTextSelect(216, 120, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[2].scalar);

    mp->graphs[0] = GUI_CreateXYGraph(COL1_TEXT, 140, 96, 96,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[0]);
    mp->graphs[1] = GUI_CreateXYGraph(112, 140, 96, 96,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[1]);
    mp->graphs[2] = GUI_CreateXYGraph(216, 140, 96, 96,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[2]);

    //Enable/Disable the relevant widgets
    update_rate_widgets(0);
    update_rate_widgets(1);
}


static void show_complex()
{
    //Row 1
    if (! mp->expoObj[0]) {
        mp->firstObj = GUI_CreateLabel(COL1_TEXT, 40, NULL, BOLD_FONT, "Mixers:");
        GUI_CreateTextSelect(COL1_VALUE, 40, TEXTSELECT_96, 0x0000, NULL, set_nummixers_cb, NULL);
        GUI_CreateLabel(COL2_TEXT, 40, NULL, BOLD_FONT, "Page:");
        GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, NULL, set_mixernum_cb, NULL);
    } else {
        GUI_RemoveHierObjects(mp->expoObj[0]);
    }
    //Row 2
    mp->expoObj[0] = GUI_CreateLabel(COL1_TEXT, 64, NULL, DEFAULT_FONT, "Switch:");
    GUI_CreateTextSelect(COL1_VALUE, 64, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->cur_mixer->sw);
    GUI_CreateLabel(COL2_TEXT, 64, NULL, DEFAULT_FONT, "Mux:");
    GUI_CreateTextSelect(COL2_VALUE, 64, TEXTSELECT_96, 0x0000, NULL, set_mux_cb, NULL);
    //Row 3
    GUI_CreateLabel(COL1_TEXT, 98, NULL, DEFAULT_FONT, "Src:");
    GUI_CreateTextSelect(COL1_VALUE, 98, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->cur_mixer->src);
    //Row 4
    GUI_CreateLabel(COL1_TEXT, 122, NULL, DEFAULT_FONT, "Curve:");
    GUI_CreateTextSelect(COL1_VALUE, 122, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, mp->cur_mixer);
    //Row 5
    GUI_CreateLabel(COL1_TEXT, 156, NULL, DEFAULT_FONT, "Scale:");
    GUI_CreateTextSelect(COL1_VALUE, 156, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->cur_mixer->scalar);
    //Row 6
    GUI_CreateLabel(COL1_TEXT, 180, NULL, DEFAULT_FONT, "Offset:");
    GUI_CreateTextSelect(COL1_VALUE, 180, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->cur_mixer->offset);
    mp->graphs[1] = GUI_CreateBarGraph(COL2_TEXT, 88, 10, 120,
                              CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL,
                              eval_chan_cb, NULL);
    mp->graphs[0] = GUI_CreateXYGraph(192, 88, 120, 120,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, mp->cur_mixer);
    //Row 7
    GUI_CreateLabel(COL1_TEXT, 216, NULL, DEFAULT_FONT, "Min:");
    GUI_CreateTextSelect(COL1_VALUE, 216, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.min);
    GUI_CreateLabel(COL2_TEXT, 216, NULL, DEFAULT_FONT, "Max:");
    GUI_CreateTextSelect(COL2_VALUE, 216, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.max);
}

s16 eval_mixer_cb(s16 xval, void * data)
{
    struct Mixer *mix = (struct Mixer *)data;
    if (MIXER_SRC_IS_INV(mix->src))
        xval = -xval;
    s16 yval = CURVE_Evaluate(xval, &mix->curve);
    yval = yval * mix->scalar / 100 + PCT_TO_RANGE(mix->offset);

    if (yval > PCT_TO_RANGE(mp->limit.max))
        yval = PCT_TO_RANGE(mp->limit.max);
    else if (yval < PCT_TO_RANGE(mp->limit.min))
        yval = PCT_TO_RANGE(mp->limit.min);

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
    MIXER_CreateCyclicInputs(mp->raw);
    struct Mixer *mix = MIXER_GetAllMixers();
    for (i = 0; i < NUM_MIXERS; i++) {
        if(MIXER_SRC(mix->src) != 0 && mix->dest != mp->cur_mixer->dest)
            MIXER_ApplyMixer(mix, mp->raw);
    }
    for (i = 0; i < mp->num_mixers; i++)
        MIXER_ApplyMixer(&mp->mixer[i], mp->raw);
    s16 value = MIXER_ApplyLimits(mp->cur_mixer->dest, &mp->limit, mp->raw, APPLY_ALL);
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
    *value = GUI_TextSelectHelper(*value, -100, 100, dir, 1, 5, NULL);
    sprintf(mp->tmpstr, "%d", *value);
    return mp->tmpstr;
}

void sync_mixers()
{
    switch(mp->cur_template) {
    case MIXERTEMPLATE_NONE:
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
    s8 min = (value == &mp->limit.max) ? mp->limit.min : -100;
    s8 max = (value == &mp->limit.min) ? mp->limit.max : 100;
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
    mp->cur_mixer->mux = GUI_TextSelectHelper(mp->cur_mixer->mux, MUX_REPLACE, MUX_ADD, dir, 1, 1, &changed);
    if (changed) {
        redraw_graphs();
        sync_mixers();
    }
    switch(mp->cur_mixer->mux) {
        case MUX_REPLACE:  return "replace";
        case MUX_MULTIPLY: return "mult";
        case MUX_ADD:      return "add";
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
                     1 + sizeof(mp->mixer) / sizeof(struct Mixer),
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
        show_complex();
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
    *source = GUI_TextSelectHelper(MIXER_SRC(*source), 0, NUM_INPUTS + NUM_CHANNELS, dir, 1, 1, &changed);
    MIXER_SET_SRC_INV(*source, is_neg);
    if (changed) {
        sync_mixers();
        redraw_graphs();
    }
    GUI_TextSelectEnablePress(obj, MIXER_SRC(*source));
    return MIXER_SourceName(mp->tmpstr, *source);
}

const char *set_drsource_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    u8 is_neg = MIXER_SRC_IS_INV(*source);
    u8 changed;
    u8 oldsrc = *source;
    *source = GUI_TextSelectHelper(MIXER_SRC(*source), 0, NUM_INPUTS + NUM_CHANNELS, dir, 1, 1, &changed);
    MIXER_SET_SRC_INV(*source, is_neg);
    if (changed) {
        sync_mixers();
        if ((!! MIXER_SRC(oldsrc)) ^ (!! MIXER_SRC(*source))) {
            if(data == &mp->mixer[1].sw)
                update_rate_widgets(0);
            else if(data == &mp->mixer[2].sw)
                update_rate_widgets(1);
        } else {    
            redraw_graphs();
        }
    }
    GUI_TextSelectEnablePress(obj, MIXER_SRC(*source));
    return MIXER_SourceName(mp->tmpstr, *source);
}

static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
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
    struct Mixer *mix = (struct Mixer *)data;
    if (mix->curve.type >= CURVE_EXPO) {
        memset(mp->graphs, 0, sizeof(mp->graphs));
        MIXPAGE_EditCurves(&mix->curve, graph_cb);
    }
}

static void okcancel_cb(guiObject_t *obj, void *data)
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
    PAGE_MixerInit(0);
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
        GUI_Redraw(mp->graphs[1]);
        GUI_Redraw(mp->graphs[2]);
    case MIXERTEMPLATE_COMPLEX:
    case MIXERTEMPLATE_SIMPLE:
        GUI_Redraw(mp->graphs[0]);
        break;
    case MIXERTEMPLATE_NONE: break;
    }
}
