#ifndef _MIXER_PAGE_H_
#define _MIXER_PAGE_H_

#include "mixer.h"
#include "gui/gui.h"

struct curve_edit {
    struct Curve curve;
    struct Curve *curveptr;
    void(*parent)(void);
    s8 pointnum;
    guiObject_t *graph;
    guiObject_t *value;
    guiObject_t *pointsel;
    
};

struct mixer_page {
    u8 modifying_template;
    u8 top_channel;
    guiObject_t *firstObj;
    struct Mixer mixer[5];
    struct Mixer *cur_mixer;
    struct Limit limit;
    guiObject_t *graph;
    u8 channel;
    u8 num_mixers;
    u8 num_complex_mixers;
    u8 link_curves;
    char tmpstr[10];
    s16 raw[NUM_INPUTS + NUM_CHANNELS + 1];

    enum TemplateType cur_template;

    struct curve_edit edit;
};

extern const char *channel_name[];

const char *PAGEMIX_SetNumberCB(guiObject_t *obj, int dir, void *data);
void MIXPAGE_EditCurves(struct Curve *curve, void *data);
const char *MIXPAGE_TemplateName(enum TemplateType template);
void MIXPAGE_ChangeTemplate();
const char *MIXPAGE_SourceName(u8 src);
void MIXPAGE_EditLimits();

#endif

