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

/*
struct Mixer mixer[5];
struct Mixer *cur_mixer;
struct Limit limit;
guiObject_t *graph;
u8 channel;
u8 num_mixers;
u8 num_complex_mixers;
u8 link_curves;
static char tmpstr[10];
static s16 raw[NUM_INPUTS + NUM_CHANNELS + 1];

static char input_str[ENTRIES_PER_PAGE][6];
static char switch_str[ENTRIES_PER_PAGE][7];
static enum TemplateType cur_template;

static const char *inp[] = {
    "THR", "RUD", "ELE", "AIL",
    "", "D/R", "D/R-C1", "D/R-C2"
};

static u8 modifying_template;

*/

static const char *templatetype_cb(guiObject_t *obj, int value, void *data);
static void sync_mixers();
static const char *set_number100_cb(guiObject_t *obj, int dir, void *data);
static s16 eval_mixer_cb(s16 xval, void * data);
static u8 curpos_cb(s16 *x, s16 *y, u8 pos, void *data);

static void show_none();
static void show_simple();
static void show_expo_dr();
static void show_complex();


const char *MIXPAGE_TemplateName(enum TemplateType template)
{
    switch(template) {
    case MIXERTEMPLATE_NONE :   return "None";
    case MIXERTEMPLATE_SIMPLE:  return "Simple";
    case MIXERTEMPLATE_EXPO_DR: return "Expo & D/R";
    case MIXERTEMPLATE_COMPLEX: return "Complex";
    default:                    return "Unknown";
    }
}

void MIXPAGE_ChangeTemplate()
{
    mp->cur_mixer = mp->mixer;
    sync_mixers();
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
        MIXPAGE_ChangeTemplate();
        return "";
    }
    return MIXPAGE_TemplateName(mp->cur_template);
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
    GUI_CreateLabel(COL1_TEXT, 10, NULL, TITLE_FONT, (void *)channel_name[mp->channel]);
    GUI_CreateTextSelect(COL1_VALUE, 8, TEXTSELECT_96, 0x0000, NULL, templatetype_cb, (void *)((long)mp->channel));
    GUI_CreateButton(160, 4, BUTTON_96, "Cancel", 0x0000, okcancel_cb, (void *)0);
    GUI_CreateButton(264, 4, BUTTON_48, "OK", 0x0000, okcancel_cb, (void *)1);
}

static void show_none()
{
    GUI_RemoveAllObjects();
    mp->graph = NULL;
    //Row 0
    show_titlerow();
}

static void show_simple()
{
    GUI_RemoveAllObjects();
    //Row 0
    show_titlerow();
    //Row 1
    GUI_CreateLabel(COL1_TEXT, 48, NULL, DEFAULT_FONT, "Src:");
    GUI_CreateTextSelect(COL1_VALUE, 48, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->mixer[0].src);
    GUI_CreateLabel(COL2_TEXT, 48, NULL, DEFAULT_FONT, "Curve:");
    GUI_CreateTextSelect(COL2_VALUE, 48, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[0]);
    //Row 2
    GUI_CreateLabel(COL1_TEXT, 72, NULL, DEFAULT_FONT, "Scale:");
    GUI_CreateTextSelect(COL1_VALUE, 72, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[0].scalar);
    GUI_CreateLabel(COL2_TEXT, 72, NULL, DEFAULT_FONT, "Offset:");
    GUI_CreateTextSelect(COL2_VALUE, 72, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[0].offset);
    //Row 4
    GUI_CreateLabel(COL1_TEXT, 96, NULL, DEFAULT_FONT, "Max:");
    GUI_CreateTextSelect(COL1_VALUE, 96, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.max);
    GUI_CreateLabel(COL2_TEXT, 96, NULL, DEFAULT_FONT, "Min:");
    GUI_CreateTextSelect(COL2_VALUE, 96, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.min);
    //Row 5
    mp->graph = GUI_CreateXYGraph(104, 120, 112, 112,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[0].curve);
}

void toggle_link_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if(data)
       mp->link_curves ^= 0x02;
    else
       mp->link_curves ^= 0x01;
    show_expo_dr();
}

static void show_expo_dr()
{
    GUI_RemoveAllObjects();
    sync_mixers();
    //Row 0
    show_titlerow();
    //Row 1
    GUI_CreateLabel(40, 32, NULL, DEFAULT_FONT, "Src");
    GUI_CreateLabel(132, 32, NULL, DEFAULT_FONT, "Switch1");
    GUI_CreateLabel(236, 32, NULL, DEFAULT_FONT, "Switch2");
    //Row 2
    GUI_CreateTextSelect(COL1_TEXT, 48, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->mixer[0].src);
    GUI_CreateTextSelect(112, 48, TEXTSELECT_96, 0x0000, sourceselect_cb, set_drsource_cb, &mp->mixer[1].sw);
    GUI_CreateTextSelect(216, 48, TEXTSELECT_96, 0x0000, sourceselect_cb, set_drsource_cb, &mp->mixer[2].sw);
    //Row 3
    GUI_CreateLabel(18, 74, NULL, DEFAULT_FONT, "High-Rate");
    if (mp->mixer[1].sw)
        GUI_CreateButton(115, 70, BUTTON_96, "Mid-Rate", 0x0000, toggle_link_cb, (void *)0);
    if (mp->mixer[2].sw)
        GUI_CreateButton(219, 70, BUTTON_96, "Low-Rate", 0x0000, toggle_link_cb, (void *)1);
    //Row 4
    GUI_CreateTextSelect(COL1_TEXT, 100, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[0]);
    if (mp->mixer[1].sw) {
        if((mp->link_curves & 0x01)) {
            GUI_CreateLabel(140, 102, NULL, DEFAULT_FONT, "Linked");
        } else {
            GUI_CreateTextSelect(112, 100, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[1]);
        }
    }
    if (mp->mixer[2].sw) {
        if((mp->link_curves & 0x02)) {
            GUI_CreateLabel(244, 102, NULL, DEFAULT_FONT, "Linked");
        } else {
            GUI_CreateTextSelect(216, 100, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[2]);
        }
    }
    //Row 5
    GUI_CreateLabel(COL1_TEXT, 122, NULL, DEFAULT_FONT, "Scale:");
    GUI_CreateTextSelect(40, 120, TEXTSELECT_64, 0x0000, NULL, set_number100_cb, &mp->mixer[0].scalar);
    if (MIX_SRC(mp->mixer[1].sw))
        GUI_CreateTextSelect(112, 120, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[1].scalar);
    if (MIX_SRC(mp->mixer[2].sw))
        GUI_CreateTextSelect(216, 120, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[2].scalar);
    mp->graph = GUI_CreateXYGraph(COL1_TEXT, 140, 96, 96,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[0].curve);
}


static void show_complex()
{
    GUI_RemoveAllObjects();
    //Row 0
    show_titlerow();
    //Row 1
    GUI_CreateLabel(COL1_TEXT, 40, NULL, DEFAULT_FONT, "Src:");
    GUI_CreateTextSelect(COL1_VALUE, 40, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->cur_mixer->src);
    GUI_CreateLabel(COL2_TEXT, 40, NULL, DEFAULT_FONT, "Curve:");
    GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, mp->cur_mixer);
    //Row 2
    GUI_CreateLabel(COL1_TEXT, 66, NULL, DEFAULT_FONT, "Scale:");
    GUI_CreateTextSelect(COL1_VALUE, 66, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->cur_mixer->scalar);
    GUI_CreateLabel(COL2_TEXT, 66, NULL, DEFAULT_FONT, "Offset:");
    GUI_CreateTextSelect(COL2_VALUE, 66, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->cur_mixer->offset);
    //Row 3
    GUI_CreateLabel(COL1_TEXT, 92, NULL, DEFAULT_FONT, "Max:");
    GUI_CreateTextSelect(COL1_VALUE, 92, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.max);
    GUI_CreateLabel(COL2_TEXT, 92, NULL, DEFAULT_FONT, "Min:");
    GUI_CreateTextSelect(COL2_VALUE, 92, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.min);
    //Row 4
    GUI_CreateLabel(COL1_TEXT, 118, NULL, DEFAULT_FONT, "Switch:");
    GUI_CreateTextSelect(COL1_VALUE, 118, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->cur_mixer->sw);
    mp->graph = GUI_CreateXYGraph(182, 118, 114, 114,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->cur_mixer->curve);
    //Row 5
    GUI_CreateLabel(COL1_TEXT, 144, NULL, DEFAULT_FONT, "Mux:");
    GUI_CreateTextSelect(COL1_VALUE, 144, TEXTSELECT_96, 0x0000, NULL, set_mux_cb, NULL);
    //Row 6
    GUI_CreateLabel(COL1_TEXT, 170, NULL, DEFAULT_FONT, "Mixers:");
    GUI_CreateTextSelect(COL1_VALUE, 170, TEXTSELECT_96, 0x0000, NULL, set_nummixers_cb, NULL);
    //Row 6
    GUI_CreateLabel(COL1_TEXT, 196, NULL, DEFAULT_FONT, "Page:");
    GUI_CreateTextSelect(COL1_VALUE, 196, TEXTSELECT_96, 0x0000, NULL, set_mixernum_cb, NULL);
}

s16 eval_mixer_cb(s16 xval, void * data)
{
    (void) data;
    int i;
    u8 src = MIX_SRC(mp->cur_mixer->src);
    s16 oldval = mp->raw[src];
    if (src <= NUM_TX_INPUTS)
        mp->raw[src] = xval;
    MIX_CreateCyclicInputs(mp->raw);
    if (src > NUM_TX_INPUTS)
        mp->raw[src] = xval;
    struct Mixer *mix = MIX_GetAllMixers();
    for (i = 0; i < NUM_MIXERS; i++) {
        if(MIX_SRC(mix->src) != 0 && mix->dest != mp->cur_mixer->dest)
            MIX_ApplyMixer(mix, mp->raw);
    }
    for (i = 0; i < mp->num_mixers; i++)
        MIX_ApplyMixer(&mp->mixer[i], mp->raw);
    mp->raw[src] = oldval;
    if (mp->raw[mp->cur_mixer->dest + NUM_INPUTS + 1] > CHAN_MAX_VALUE)
        return CHAN_MAX_VALUE;
    if (mp->raw[mp->cur_mixer->dest + NUM_INPUTS + 1]  < CHAN_MIN_VALUE)
        return CHAN_MIN_VALUE;
    return mp->raw[mp->cur_mixer->dest + NUM_INPUTS + 1];
}

u8 curpos_cb(s16 *x, s16 *y, u8 pos, void *data)
{
    if (pos != 0)
        return 0;
    *x = mp->raw[MIX_SRC(mp->cur_mixer->src)];
    if (*x > CHAN_MAX_VALUE)
        *x = CHAN_MAX_VALUE;
    else if (*x  < CHAN_MIN_VALUE)
        *x = CHAN_MIN_VALUE;
    *y = eval_mixer_cb(*x, data);
    return 1;
}

const char *PAGEMIX_SetNumberCB(guiObject_t *obj, int dir, void *data)
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
        if (MIX_SRC(mp->mixer[1].sw)) {
            mp->mixer[1].src    = mp->mixer[0].src;
            mp->mixer[1].dest   = mp->mixer[0].dest;
            mp->mixer[1].mux    = MUX_REPLACE;
            mp->mixer[1].offset = 0;
            if (mp->link_curves & 0x01)
                mp->mixer[1].curve = mp->mixer[0].curve;
        } else {
            mp->mixer[1].src = 0;
        }
        if (MIX_SRC(mp->mixer[2].sw)) {
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
        mp->num_mixers = 3;
        break;
    }
}

const char *set_number100_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 changed;
    s8 *value = (s8 *)data;
    *value = GUI_TextSelectHelper(*value, -100, 100, dir, 1, 5, &changed);
    sprintf(mp->tmpstr, "%d", *value);
    if (changed) {
        sync_mixers();
        GUI_Redraw(mp->graph);
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
        GUI_Redraw(mp->graph);
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
        GUI_Redraw(mp->graph);
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

const char *MIXPAGE_SourceName(u8 src)
{
    u8 is_neg = MIX_SRC_IS_INV(src);
    src = MIX_SRC(src);

    if(! src) {
        return "None";
    }
    if(src <= NUM_TX_INPUTS) {
        if (is_neg) {
            sprintf(mp->tmpstr, "!%s", tx_input_str[src - 1]);
            return mp->tmpstr;
        }
        return tx_input_str[src - 1];
    }
    if(src <= NUM_INPUTS) {
        sprintf(mp->tmpstr, "%sCYC%d", is_neg ? "!" : "", src - NUM_TX_INPUTS);
        return mp->tmpstr;
    }
    if (is_neg) {
        sprintf(mp->tmpstr, "!%s", channel_name[src - NUM_INPUTS - 1]);
        return mp->tmpstr;
    }
    return channel_name[src - NUM_INPUTS - 1];
}

const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    u8 is_neg = MIX_SRC_IS_INV(*source);
    u8 changed;
    *source = GUI_TextSelectHelper(MIX_SRC(*source), 0, NUM_INPUTS + NUM_CHANNELS, dir, 1, 1, &changed);
    MIX_SET_SRC_INV(*source, is_neg);
    if (changed) {
        GUI_Redraw(mp->graph);
        sync_mixers();
    }
    return MIXPAGE_SourceName(*source);
}

const char *set_drsource_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    u8 is_neg = MIX_SRC_IS_INV(*source);
    u8 changed;
    u8 oldsrc = *source;
    *source = GUI_TextSelectHelper(MIX_SRC(*source), 0, NUM_INPUTS + NUM_CHANNELS, dir, 1, 1, &changed);
    MIX_SET_SRC_INV(*source, is_neg);
    if (changed) {
        if ((! MIX_SRC(oldsrc) || ! MIX_SRC(*source)) && (source == &mp->mixer[1].sw || source == &mp->mixer[2].sw)) {
            show_expo_dr();
        } else {    
            GUI_Redraw(mp->graph);
        }
    }
    return MIXPAGE_SourceName(*source);
}

static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    struct Mixer *mix = (struct Mixer *)data;
    mix->curve.type = GUI_TextSelectHelper(mix->curve.type, 0, CURVE_MAX, dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(mp->graph);
        sync_mixers();
    }
    return CURVE_GetName(&mix->curve);
}

void sourceselect_cb(guiObject_t *obj, void *data)
{
    u8 *source = (u8 *)data;
    MIX_SET_SRC_INV(*source, ! MIX_SRC_IS_INV(*source));
    GUI_Redraw(obj);
    GUI_Redraw(mp->graph);
}

void curveselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    struct Mixer *mix = (struct Mixer *)data;
    void *func;
    switch (mp->cur_template) {
        case MIXERTEMPLATE_EXPO_DR: func = show_expo_dr; break;
        case MIXERTEMPLATE_SIMPLE:  func = show_simple; break;
        case MIXERTEMPLATE_COMPLEX: func = show_complex; break;
        default: return;
    }
    MIXPAGE_EditCurves(&mix->curve, func);   
}

static void okcancel_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if (data) {
        //Save mixer here
        MIX_SetLimit(mp->channel, &mp->limit);
        MIX_SetTemplate(mp->channel, mp->cur_template);
        MIX_SetMixers(mp->mixer, mp->num_mixers);
    }
    GUI_RemoveAllObjects();
    mp->graph = NULL;
    PAGE_MixerInit(0);
}

static u8 touch_cb(s16 x, s16 y, void *data)
{
    (void)x;
    (void)y;
    struct Curve *curve = (struct Curve *)data;
    void *func;
    switch (mp->cur_template) {
        case MIXERTEMPLATE_EXPO_DR: func = show_expo_dr; break;
        case MIXERTEMPLATE_SIMPLE:  func = show_simple; break;
        case MIXERTEMPLATE_COMPLEX: func = show_complex; break;
        default: return 0;
    }
    MIXPAGE_EditCurves(curve, func);   
    return 1;
}
