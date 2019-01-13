#include "CuTest.h"

void TestMixerReorder(CuTest *t)
{
    u8 list[NUM_CHANNELS];
    u8 order[] = {4, 1, 3, 6, 2, 5};
    memset(&Model, 0, sizeof(Model));
    for (unsigned i = 0; i < NUM_CHANNELS; i++) {
        list[i] = i+1;
    }
    memcpy(list, order, sizeof(order));
    for(unsigned i = 0; i < 2*sizeof(order); i++) {
        Model.mixers[i].src = 1 + i;
        Model.mixers[i].dest = i / 2;
    }

    reorder_mixers_by_list(list);
    
    for(unsigned i = 0; i < 2*sizeof(order); i++) {
        unsigned dest = i / 2;
        unsigned src = 1 + 2 * (list[dest]-1) + (i % 2);
        CuAssertIntEquals(t, src, Model.mixers[i].src);
        CuAssertIntEquals(t, dest, Model.mixers[i].dest);
    }
}

void TestLimitsReorder(CuTest *t)
{
    u8 list[NUM_CHANNELS];
    u8 order[] = {4, 1, 3, 6, 2, 5, NUM_OUT_CHANNELS+1, 7};
    memset(&Model, 0, sizeof(Model));
    for (unsigned i = 0; i < NUM_CHANNELS; i++) {
        list[i] = i+1;
        Model.limits[i].max = i;
        Model.templates[i] = i;
    }
    memcpy(list, order, sizeof(order));

    reorder_limits_by_list(list);
    
    for(unsigned i = 0; i < sizeof(order); i++) {
        unsigned templ = list[i]-1;
        unsigned max = templ < NUM_OUT_CHANNELS ? templ : 150;
        CuAssertIntEquals(t, max, Model.limits[i].max);
        CuAssertIntEquals(t, templ, Model.templates[i]);
    }
}
