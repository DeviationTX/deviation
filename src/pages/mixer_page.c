/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Foobar is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "target.h"
#include "pages.h"
#include "gui/gui.h"
#include "mixer.h"
#include "mixer_page.h"

struct Mixer mixer[5];
struct Mixer *cur_mixer;
struct Limit limit;
guiObject_t *graph;
u8 channel;
u8 num_mixers;
u8 num_complex_mixers;
u8 link_curves;
static char tmpstr[5];
static s16 raw[NUM_INPUTS + 1];

static const char *channel_name[] = {
    "Ch1", "Ch2", "Ch3", "Ch4",
    "Ch5", "Ch6", "Ch7", "Ch8",
    "Ch9", "Ch10", "Ch11", "Ch12",
    "Ch13", "Ch14", "Ch15", "Ch16"
};

#define ENTRIES_PER_PAGE 8

static char input_str[ENTRIES_PER_PAGE][6];
static char switch_str[ENTRIES_PER_PAGE][7];
static enum TemplateType cur_template;

static const char *template_name(enum TemplateType template);
static const char *templatetype_cb(guiObject_t *obj, int value, void *data);
static void templateselect_cb(guiObject_t *obj, void *data);
static void sync_mixers();
static const char *set_number100_cb(guiObject_t *obj, int dir, void *data);
static s16 eval_mixer_cb(s16 xval, void * data);
static u8 curpos_cb(s16 *x, s16 *y, u8 pos, void *data);

static void show_none();
static void show_simple();
static void show_expo_dr();
static void show_complex();

static const char *inp[] = {
    "THR", "RUD", "ELE", "AIL",
    "", "D/R", "D/R-C1", "D/R-C2"
};

static u8 modifying_template;

void PAGE_MixerInit(int page)
{
    int init_y = 16;
    int i;
    modifying_template = 0;
    for (i = 0; i < ENTRIES_PER_PAGE; i++) {
        void *ptr = (void *)((long)i);
        int row = init_y + 28 * i;
        GUI_CreateLabel(10, row, channel_name[ENTRIES_PER_PAGE * page + i], 0x0000);
        strcpy(input_str[i], (i < 4) ? inp[i] : "");
        GUI_CreateLabel(40, row, input_str[i], 0x0000);
        GUI_CreateButton(100, row - 4, BUTTON_90, template_name(MIX_GetTemplate(i)), 0x0000, templateselect_cb, ptr);
        strcpy(switch_str[i], (i < 4) ? inp[i+4] : "");
        GUI_CreateLabel(240, row, switch_str[i], 0x0000);
    }
    GUI_DrawScreen();
}

void PAGE_MixerEvent()
{
    if(graph && cur_mixer) {
        u8 changed = 0;
        s16 chan;
        int i;
        for (i = 1; i <= NUM_TX_INPUTS; i++) {
            chan = CHAN_ReadInput(i);
            if (chan != raw[i]) {
                raw[i] = chan;
                changed = 1;
            }
        }
        if (changed)
            GUI_Redraw(graph);
    }
}

int PAGE_MixerCanChange()
{
    return ! modifying_template;
}

static const char *template_name(enum TemplateType template)
{
    switch(template) {
    case MIXERTEMPLATE_NONE :   return "None";
    case MIXERTEMPLATE_SIMPLE:  return "Simple";
    case MIXERTEMPLATE_EXPO_DR: return "Expo & D/R";
    case MIXERTEMPLATE_COMPLEX: return "Complex";
    default:                    return "Unknown";
    }
}

static void change_template()
{
    cur_mixer = mixer;
    sync_mixers();
    switch(cur_template)  {
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
    cur_template = GUI_TextSelectHelper(cur_template, 0, MIXERTEMPLATE_MAX, dir, 1, 1, &changed);
    if (changed) {
        change_template();
        return "";
    }
    return template_name(cur_template);
}

static void templateselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    long idx = (long)data;
    u8 i;

    cur_template = MIX_GetTemplate(idx);
    modifying_template = 1;
    MIX_GetLimit(idx, &limit);
    channel = idx;
    num_complex_mixers = 0;
    link_curves = 0xff;
    for(i = 0; i < sizeof(mixer) / sizeof(struct Mixer); i++)
        MIX_InitMixer(mixer + i, idx);

    if (cur_template != MIXERTEMPLATE_NONE) {
        num_complex_mixers = MIX_GetMixers(idx, mixer, sizeof(mixer) / sizeof(struct Mixer));
    }
    change_template();
}

static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data);
void curveselect_cb(guiObject_t *obj, void *data);
const char *set_source_cb(guiObject_t *obj, int dir, void *data);
const char *set_drsource_cb(guiObject_t *obj, int dir, void *data);
const char *set_mux_cb(guiObject_t *obj, int dir, void *data);
const char *set_nummixers_cb(guiObject_t *obj, int dir, void *data);
const char *set_mixernum_cb(guiObject_t *obj, int dir, void *data);
static void okcancel_cb(guiObject_t *obj, void *data);

static void show_titlerow()
{
    GUI_CreateLabel(10, 10, channel_name[channel], 0x0000);
    GUI_CreateTextSelect(40, 10, TEXTSELECT_96, 0x0000, NULL, templatetype_cb, (void *)((long)channel));
    GUI_CreateButton(150, 6, BUTTON_90, "Cancel", 0x0000, okcancel_cb, (void *)0);
    GUI_CreateButton(264, 6, BUTTON_45, "Ok", 0x0000, okcancel_cb, (void *)1);
}

static void show_none()
{
    GUI_RemoveAllObjects();
    graph = NULL;
    //Row 0
    show_titlerow();
}

#define COL1_TEXT   10
#define COL1_VALUE  60
#define COL2_TEXT  170
#define COL2_VALUE 214
static void show_simple()
{
    GUI_RemoveAllObjects();
    //Row 0
    show_titlerow();
    //Row 1
    GUI_CreateLabel(COL1_TEXT, 40, "Src:", 0x0000);
    GUI_CreateTextSelect(COL1_VALUE, 40, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[0].src);
    GUI_CreateLabel(COL2_TEXT, 40, "Curve:", 0x0000);
    GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mixer[0]);
    //Row 2
    GUI_CreateLabel(COL1_TEXT, 66, "Scale:", 0x0000);
    GUI_CreateTextSelect(COL1_VALUE, 66, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mixer[0].scaler);
    GUI_CreateLabel(COL2_TEXT, 66, "Offset:", 0x0000);
    GUI_CreateTextSelect(COL2_VALUE, 66, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mixer[0].offset);
    //Row 4
    GUI_CreateLabel(COL1_TEXT, 92, "Max:", 0x0000);
    GUI_CreateTextSelect(COL1_VALUE, 92, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &limit.max);
    GUI_CreateLabel(COL2_TEXT, 92, "Min:", 0x0000);
    GUI_CreateTextSelect(COL2_VALUE, 92, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &limit.min);
    //Row 5
    graph = GUI_CreateXYGraph(COL1_TEXT, 118, 300, 112,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, NULL, &mixer[0].curve);
}

void toggle_link_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if(data)
       link_curves ^= 0x02;
    else
       link_curves ^= 0x01;
    show_expo_dr();
}

static void show_expo_dr()
{
    GUI_RemoveAllObjects();
    sync_mixers();
    //Row 0
    show_titlerow();
    //Row 1
    GUI_CreateLabel(40, 32, "Src", 0x0000);
    GUI_CreateLabel(132, 32, "Switch1", 0x0000);
    GUI_CreateLabel(236, 32, "Switch2", 0x0000);
    //Row 2
    GUI_CreateTextSelect(8, 48, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[0].src);
    GUI_CreateTextSelect(112, 48, TEXTSELECT_96, 0x0000, NULL, set_drsource_cb, &mixer[1].sw);
    GUI_CreateTextSelect(216, 48, TEXTSELECT_96, 0x0000, NULL, set_drsource_cb, &mixer[2].sw);
    //Row 3
    GUI_CreateLabel(18, 74, "High-Rate", 0x0000);
    if (mixer[1].sw)
        GUI_CreateButton(115, 70, BUTTON_90, "Med-Rate", 0x0000, toggle_link_cb, (void *)0);
    if (mixer[2].sw)
        GUI_CreateButton(219, 70, BUTTON_90, "Low-Rate", 0x0000, toggle_link_cb, (void *)1);
    //Row 4
    GUI_CreateTextSelect(8, 100, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mixer[0]);
    if (mixer[1].sw) {
        if((link_curves & 0x01)) {
            GUI_CreateLabel(140, 102, "Linked", 0x0000);
        } else {
            GUI_CreateTextSelect(112, 100, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mixer[1]);
        }
    }
    if (mixer[2].sw) {
        if((link_curves & 0x02)) {
            GUI_CreateLabel(244, 102, "Linked", 0x0000);
        } else {
            GUI_CreateTextSelect(216, 100, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mixer[2]);
        }
    }
    //Row 5
    GUI_CreateLabel(8, 122, "Scale:", 0x0000);
    GUI_CreateTextSelect(40, 120, TEXTSELECT_64, 0x0000, NULL, set_number100_cb, &mixer[0].scaler);
    if (mixer[1].sw)
        GUI_CreateTextSelect(112, 120, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mixer[1].scaler);
    if (mixer[2].sw)
        GUI_CreateTextSelect(216, 120, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mixer[2].scaler);
    graph = GUI_CreateXYGraph(COL1_TEXT, 150, 86, 86,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, NULL, &mixer[0].curve);
}


static void show_complex()
{
    GUI_RemoveAllObjects();
    //Row 0
    show_titlerow();
    //Row 1
    GUI_CreateLabel(COL1_TEXT, 40, "Src:", 0x0000);
    GUI_CreateTextSelect(COL1_VALUE, 40, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &cur_mixer->src);
    GUI_CreateLabel(COL2_TEXT, 40, "Curve:", 0x0000);
    GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, cur_mixer);
    //Row 2
    GUI_CreateLabel(COL1_TEXT, 66, "Scale:", 0x0000);
    GUI_CreateTextSelect(COL1_VALUE, 66, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &cur_mixer->scaler);
    GUI_CreateLabel(COL2_TEXT, 66, "Offset:", 0x0000);
    GUI_CreateTextSelect(COL2_VALUE, 66, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &cur_mixer->offset);
    //Row 3
    GUI_CreateLabel(COL1_TEXT, 92, "Max:", 0x0000);
    GUI_CreateTextSelect(COL1_VALUE, 92, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &limit.max);
    GUI_CreateLabel(COL2_TEXT, 92, "Min:", 0x0000);
    GUI_CreateTextSelect(COL2_VALUE, 92, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &limit.min);
    //Row 4
    GUI_CreateLabel(COL1_TEXT, 118, "Switch:", 0x0000);
    GUI_CreateTextSelect(COL1_VALUE, 118, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &cur_mixer->sw);
    graph = GUI_CreateXYGraph(COL2_TEXT, 118, 140, 112,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, NULL, &cur_mixer->curve);
    //Row 5
    GUI_CreateLabel(COL1_TEXT, 144, "Mux:", 0x0000);
    GUI_CreateTextSelect(COL1_VALUE, 144, TEXTSELECT_96, 0x0000, NULL, set_mux_cb, NULL);
    //Row 6
    GUI_CreateLabel(COL1_TEXT, 170, "Mixers:", 0x0000);
    GUI_CreateTextSelect(COL1_VALUE, 170, TEXTSELECT_96, 0x0000, NULL, set_nummixers_cb, NULL);
    //Row 6
    GUI_CreateLabel(COL1_TEXT, 196, "Page:", 0x0000);
    GUI_CreateTextSelect(COL1_VALUE, 196, TEXTSELECT_96, 0x0000, NULL, set_mixernum_cb, NULL);
}

s16 eval_mixer_cb(s16 xval, void * data)
{
    (void) data;
    int i;
    s16 mixed[NUM_CHANNELS];
    s16 oldval = raw[cur_mixer->src];
    raw[cur_mixer->src] = xval;
    MIX_EvalMixers(raw, mixed);
    for (i = 0; i < num_mixers; i++)
        MIX_ApplyMixer(&mixer[i], raw, mixed);
    raw[cur_mixer->src] = oldval;
    if (mixed[cur_mixer->dest] > CHAN_MAX_VALUE)
        return CHAN_MAX_VALUE;
    if (mixed[cur_mixer->dest]  < CHAN_MIN_VALUE)
        return CHAN_MIN_VALUE;
    return mixed[cur_mixer->dest];
}

u8 curpos_cb(s16 *x, s16 *y, u8 pos, void *data)
{
    if (pos != 0)
        return 0;
    *x = raw[cur_mixer->src];
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
    sprintf(tmpstr, "%d", *value);
    return tmpstr;
}

void sync_mixers()
{
    switch(cur_template) {
    case MIXERTEMPLATE_NONE:
        num_mixers = 0;
        break;
    case MIXERTEMPLATE_SIMPLE:
        mixer[0].sw = 0;
        mixer[0].mux = MUX_REPLACE;
        num_mixers = 1;
        break;
    case MIXERTEMPLATE_COMPLEX:
        num_mixers = num_complex_mixers;
        break;
    case MIXERTEMPLATE_EXPO_DR:
        if (mixer[1].sw) {
            mixer[1].src = mixer[0].src;
            mixer[1].dest = mixer[0].dest;
            if (link_curves & 0x01)
                mixer[1].curve = mixer[0].curve;
            mixer[1].mux = MUX_REPLACE;
            mixer[1].offset = 0;
        } else {
            mixer[1].src = 0;
        }
        if (mixer[2].sw) {
            mixer[2].src = mixer[0].src;
            mixer[2].dest = mixer[0].dest;
            if (link_curves & 0x02)
                mixer[2].curve = mixer[0].curve;
            mixer[2].mux = MUX_REPLACE;
            mixer[2].offset = 0;
        } else {
            mixer[2].src = 0;
        }
        mixer[0].mux = MUX_REPLACE;
        mixer[0].offset = 0;
        mixer[0].sw = 0;
        num_mixers = 3;
        break;
    }
}

const char *set_number100_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 changed;
    s8 *value = (s8 *)data;
    *value = GUI_TextSelectHelper(*value, -100, 100, dir, 1, 5, &changed);
    sprintf(tmpstr, "%d", *value);
    if (changed) {
        sync_mixers();
        GUI_Redraw(graph);
    }
    return tmpstr;
}

const char *set_mux_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed;
    cur_mixer->mux = GUI_TextSelectHelper(cur_mixer->mux, MUX_REPLACE, MUX_ADD, dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(graph);
        sync_mixers();
    }
    switch(cur_mixer->mux) {
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
    num_mixers = GUI_TextSelectHelper(
                     num_mixers,
                     1 + (cur_mixer - mixer),
                     1 + sizeof(mixer) / sizeof(struct Mixer),
                     dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(graph);
        sync_mixers();
    }
    sprintf(tmpstr, "%d", num_mixers);
    return tmpstr;
}

const char *set_mixernum_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 cur = (cur_mixer - mixer) + 1;
    u8 changed;
    cur = GUI_TextSelectHelper(cur, 1, num_mixers, dir, 1, 1, &changed);
    if (changed) {
        cur_mixer = mixer + (cur - 1);
        show_complex();
    }
    sprintf(tmpstr, "%d", cur);
    return tmpstr;
}


const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    u8 changed;
    *source = GUI_TextSelectHelper(*source, 0, NUM_INPUTS + NUM_CHANNELS, dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(graph);
        sync_mixers();
    }
    if(! *source) {
        return "None";
    }
    if(*source <= NUM_TX_INPUTS) {
        return tx_input_str[*source - 1];
    }
    if(*source <= NUM_INPUTS) {
        sprintf(tmpstr, "CYC%d", *source - NUM_TX_INPUTS);
        return tmpstr;
    }
    return channel_name[*source - NUM_INPUTS - 1];
}

const char *set_drsource_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    u8 changed;
    u8 oldsrc = *source;
    *source = GUI_TextSelectHelper(*source, 0, NUM_INPUTS + NUM_CHANNELS, dir, 1, 1, &changed);
    if (changed) {
        if ((! *source || ! oldsrc) && (source == &mixer[1].sw || source == &mixer[2].sw)) {
            show_expo_dr();
        } else {    
            GUI_Redraw(graph);
        }
    }
    if(! *source) {
        return "None";
    }
    if(*source <= NUM_TX_INPUTS) {
        return tx_input_str[*source - 1];
    }
    if(*source <= NUM_INPUTS) {
        sprintf(tmpstr, "CYC%d", *source - NUM_TX_INPUTS);
        return tmpstr;
    }
    return channel_name[*source - NUM_INPUTS - 1];
}

static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    struct Mixer *mix = (struct Mixer *)data;
    mix->curve.type = GUI_TextSelectHelper(mix->curve.type, 0, CURVE_MAX, dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(graph);
        sync_mixers();
    }
    return CURVE_GetName(&mix->curve);
}

void curveselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    struct Mixer *mix = (struct Mixer *)data;
    void *func;
    switch (cur_template) {
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
        MIX_SetLimit(channel, &limit);
        MIX_SetTemplate(channel, cur_template);
        MIX_SetMixers(mixer, num_mixers);
    }
    GUI_RemoveAllObjects();
    graph = NULL;
    PAGE_MixerInit(0);
}
