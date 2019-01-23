#include "CuTest.h"

void TestMixerPageReorder(CuTest *t)
{
    u8 list[NUM_CHANNELS];
    u8 order[] = {4, 1, 3, 6, 2, 5, NUM_OUT_CHANNELS+1, 7};
    memset(&Model, 0, sizeof(Model));
    for (unsigned i = 0; i < NUM_CHANNELS; i++) {
        list[i] = i+1;
        Model.mixers[i].src = 1 + i;
        Model.mixers[i].dest = i;
        Model.mixers[i + NUM_CHANNELS].src = 2 + i;
        Model.mixers[i + NUM_CHANNELS].dest = i;
        // These template values are invalid but will be passed through and are easy to check
        Model.templates[i] = MIXERTEMPLATE_CYC3 + 1 + i;
        if (i < NUM_OUT_CHANNELS) {
            Model.limits[i].max = i;
        }
    }
    memcpy(list, order, sizeof(order));
    list[16] = 8;  // list must be a 1:1 mapping

    reorder_return_cb(list);

    printf("%d %d\n", NUM_CHANNELS, NUM_OUT_CHANNELS+1);
    
    for(unsigned i = 0; i < 2*sizeof(order); i++) {
        unsigned dest = i / 2;
        unsigned src = 1 + (list[dest]-1) + (i % 2);
        printf("%d: src %d == %d   dest %d == %d\n", i, src, Model.mixers[i].src, dest, Model.mixers[i].dest);
    }
    for(unsigned i = 0; i < 2*sizeof(order); i++) {
        unsigned dest = i / 2;
        unsigned src = 1 + (list[dest]-1) + (i % 2);
        CuAssertIntEquals(t, src, Model.mixers[i].src);
        CuAssertIntEquals(t, dest, Model.mixers[i].dest);
    }

    for(unsigned i = 0; i < sizeof(order); i++) {
        unsigned templ = list[i]-1 + MIXERTEMPLATE_CYC3 + 1;
        unsigned max = list[i]-1 < NUM_OUT_CHANNELS ? list[i]-1 : 150;
        printf("%d: templ %d == %d limit %d == %d\n", i, templ, Model.templates[i], max, Model.limits[i].max);
    }
    for(unsigned i = 0; i < sizeof(order); i++) {
        unsigned templ = list[i]-1 + MIXERTEMPLATE_CYC3 + 1;
        unsigned max = list[i]-1 < NUM_OUT_CHANNELS ? list[i]-1 : 150;
        CuAssertIntEquals(t, max, Model.limits[i].max);
        CuAssertIntEquals(t, templ, Model.templates[i]);
    }
}
