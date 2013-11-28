#ifndef _MIXER_PAGE_H_
#define _MIXER_PAGE_H_

#include "mixer.h"
#include "gui/gui.h"
#include "buttons.h"
#define NUM_COMPLEX_MIXERS 10
#define LONG_PRESS_STEP 5
#define SUBTRIM_RANGE 500  // subtrim <100 is insufficient
#define SAFETYVALUE_RANGE 200 //some motors,e.g. 005 in v120d02, are detected to need thr hold  as low as -170

struct curve_edit {
    struct buttonAction action;
    struct Curve curve;
    struct Curve *curveptr;
    void(*parent)(void);
    s8 pointnum;
    u8 reverse;
};

struct mixer_page {
    struct buttonAction action;
    u8 are_limits_changed;
    u8 top_channel;
    u8 max_scroll;
    guiObject_t *firstObj;
/*
    guiObject_t *scroll_bar;
    guiObject_t *itemObj[NUM_OUT_CHANNELS *2 + NUM_VIRT_CHANNELS];
    guiObject_t *trimObj;
    guiObject_t *safeValObj;
    guiObject_t *expoObj[10];
    guiObject_t *negscaleObj;
*/
    struct Mixer mixer[NUM_COMPLEX_MIXERS];
    struct Mixer *mixer_ptr[4];
    struct Mixer *cur_mixer;
    struct Limit limit;
/*
    guiObject_t *graphs[3];
    guiObject_t *bar;
*/
    u8 channel;
    u8 num_mixers;
    u8 num_complex_mixers;
    u8 link_curves;
    u8 entries_per_page;
    s16 raw[NUM_SOURCES + 1];
    u8 list[NUM_CHANNELS > NUM_COMPLEX_MIXERS ? NUM_CHANNELS : NUM_COMPLEX_MIXERS];

    enum TemplateType cur_template;

    struct curve_edit edit;
};

extern const char *channel_name[];

const char *PAGEMIXER_SetNumberCB(guiObject_t *obj, int dir, void *data);
void MIXPAGE_EditCurves(struct Curve *curve, void *data);
const char *MIXPAGE_TemplateName(enum TemplateType template);
const char *MIXPAGE_ChannelNameCB(guiObject_t *obj, const void *data);
const char *MIXPAGE_ChanNameProtoCB(guiObject_t *obj, const void *data);
void MIXPAGE_ChangeTemplate(int show_header);
void MIXPAGE_EditLimits();
void MIXPAGE_RedrawGraphs();

#endif

