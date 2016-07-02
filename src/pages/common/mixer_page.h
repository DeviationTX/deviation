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
    s8 pointnum;
    u8 reverse;
};

struct mixer_page {
    struct buttonAction action;
    guiObject_t *firstObj;

    struct Mixer mixer[NUM_COMPLEX_MIXERS];
    struct Mixer *mixer_ptr[4];
    struct Mixer *cur_mixer;
    struct Limit *limit;

    u8 max_scroll;
    u8 channel;
    u8 num_mixers;
    u8 num_complex_mixers;
    u8 link_curves;
    u8 entries_per_page;
    s32 raw[NUM_SOURCES + 1];
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

