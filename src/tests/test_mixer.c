#include "CuTest.h"

extern struct Transmitter Transmitter;


void TestMixerMapChannel(CuTest *t)
{
     unsigned channels[] = {INP_THROTTLE, INP_ELEVATOR, INP_AILERON, INP_RUDDER};
     Transmitter.mode = MODE_1;
     for (unsigned i = 0; i < sizeof(channels) / sizeof(channels[0]); i++) {
         unsigned expected[] = {INP_THROTTLE, INP_ELEVATOR, INP_AILERON, INP_RUDDER};
         CuAssertIntEquals(t, expected[i], MIXER_MapChannel(channels[i]));
     }
     Transmitter.mode = MODE_2;
     for (unsigned i = 0; i < sizeof(channels) / sizeof(channels[0]); i++) {
         unsigned expected[] = {INP_ELEVATOR, INP_THROTTLE, INP_AILERON, INP_RUDDER};
         CuAssertIntEquals(t, expected[i], MIXER_MapChannel(channels[i]));
     }
     Transmitter.mode = MODE_3;
     for (unsigned i = 0; i < sizeof(channels) / sizeof(channels[0]); i++) {
         unsigned expected[] = {INP_THROTTLE, INP_ELEVATOR, INP_RUDDER, INP_AILERON};
         CuAssertIntEquals(t, expected[i], MIXER_MapChannel(channels[i]));
     }
     Transmitter.mode = MODE_4;
     for (unsigned i = 0; i < sizeof(channels) / sizeof(channels[0]); i++) {
         unsigned expected[] = {INP_ELEVATOR, INP_THROTTLE, INP_RUDDER, INP_AILERON};
         CuAssertIntEquals(t, expected[i], MIXER_MapChannel(channels[i]));
     }
}

void TestApplyMixer(CuTest *t)
{
    struct Mixer initmixer = {
        .src = 1,
        .dest = 2,
        .sw = 0,
        .scalar = 100,
        .offset = 0,
        .flags = MUX_REPLACE,
    }, mixer;
    s32 rawdata[NUM_SOURCES + 1];
    s32 origvalue[NUM_SOURCES + 1];

    rawdata[0] = 999;
    rawdata[1] = 999;
    rawdata[2] = 1;
    rawdata[3] = 0;

    //Disabled mixer
    mixer = initmixer;
    mixer.src = 0;
    rawdata[3 + NUM_INPUTS] = -1;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, -1, rawdata[3 + NUM_INPUTS]);

    //Simple Mixer test
    mixer = initmixer;
    rawdata[3 + NUM_INPUTS] = -1;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, 999, rawdata[3 + NUM_INPUTS]);

    //Switch test
    mixer = initmixer;
    //Switch on
    mixer.sw = 2;
    rawdata[3 + NUM_INPUTS] = -1;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, 999, rawdata[3 + NUM_INPUTS]);
    //Switch off
    mixer.sw = 3;
    rawdata[3 + NUM_INPUTS] = -1;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, -1, rawdata[3 + NUM_INPUTS]);

}
