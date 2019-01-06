#include "CuTest.h"

extern struct Transmitter Transmitter;
extern u8 TEST_Button_InterruptLongPress();

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

void TestApplyMixerSimple(CuTest *t)
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
    s32 *origvalue = NULL;
    memset(&Model, 0, sizeof(Model));

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

   //Inverted source
    mixer = initmixer;
    MIXER_SET_SRC_INV(mixer.src, 1);
    rawdata[3 + NUM_INPUTS] = -1;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, -999, rawdata[3 + NUM_INPUTS]);

    //Multiply
    mixer = initmixer;
    mixer.flags = MUX_MULTIPLY;
    rawdata[3 + NUM_INPUTS] = CHAN_MAX_VALUE << 1;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, 1998, rawdata[3 + NUM_INPUTS]);


    //Multiply
    mixer = initmixer;
    mixer.flags = MUX_ADD;
    rawdata[3 + NUM_INPUTS] = 1000;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, 1999, rawdata[3 + NUM_INPUTS]);

    //Max
    mixer = initmixer;
    mixer.flags = MUX_MAX;
    rawdata[3 + NUM_INPUTS] = 1000;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, 1000, rawdata[3 + NUM_INPUTS]);
    rawdata[3 + NUM_INPUTS] = 998;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, 999, rawdata[3 + NUM_INPUTS]);

    //Min
    mixer = initmixer;
    mixer.flags = MUX_MIN;
    rawdata[3 + NUM_INPUTS] = 1000;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, 999, rawdata[3 + NUM_INPUTS]);
    rawdata[3 + NUM_INPUTS] = 998;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, 998, rawdata[3 + NUM_INPUTS]);

    //Delay (with 0 delay)
    mixer = initmixer;
    mixer.flags = MUX_DELAY;
    rawdata[3 + NUM_INPUTS] = -1;
    rawdata[1] = 0;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, -1, rawdata[3 + NUM_INPUTS]);

    //Max overflow test
    mixer = initmixer;
    rawdata[1] = INT16_MAX * 2;
    rawdata[3 + NUM_INPUTS] = -1;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, INT16_MAX, rawdata[3 + NUM_INPUTS]);

    //Min overflow test
    mixer = initmixer;
    rawdata[1] = INT16_MIN * 2;
    rawdata[3 + NUM_INPUTS] = -1;
    MIXER_ApplyMixer(&mixer, rawdata, origvalue);
    CuAssertIntEquals(t, INT16_MIN, rawdata[3 + NUM_INPUTS]);
}

void TestApplyMixerDelay(CuTest *t)
{
    struct Mixer mixer = {
        .src = 1,
        .dest = 2,
        .sw = 0,
        .scalar = 100,
        .offset = 0,
        .flags = MUX_DELAY,
    };
    s32 rawdata[NUM_SOURCES + 1];
    s32 origvalue;
    memset(&Model, 0, sizeof(Model));

    rawdata[1] = 1000;

    //Delay
    s32 target[] = {550, 550, 550, 550, 550, 550, 550, 400, 400, 400};
    s32 expected[] = {100, 200, 300, 400, 500, 550, 550, 450, 400, 400};
    rawdata[3 + NUM_INPUTS] = 0;
    for(unsigned i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        origvalue = rawdata[3 + NUM_INPUTS];
        rawdata[3 + NUM_INPUTS] = target[i];
        MIXER_ApplyMixer(&mixer, rawdata, &origvalue);
        CuAssertIntEquals(t, expected[i], rawdata[3 + NUM_INPUTS]);
    }
}

void TestApplyMixerTrim(CuTest *t)
{
    struct Mixer mixer = {
        .src = 1,
        .dest = 2,
        .sw = 0,
        .scalar = 100,
        .offset = 0,
        .flags = MUX_REPLACE,
    };
    s32 rawdata[NUM_SOURCES + 1];
    memset(&Model, 0, sizeof(Model));
    Model.trims[0] = (struct Trim){ .src = 1, .step = 10, .value = {5} };
    MIXER_SET_APPLY_TRIM(&mixer, 1);
    Transmitter.mode = MODE_1;

    rawdata[1] = 999;

    rawdata[3 + NUM_INPUTS] = -1;
    MIXER_ApplyMixer(&mixer, rawdata, NULL);
    CuAssertIntEquals(t, 1499, rawdata[3 + NUM_INPUTS]);
}

void TestGetTrim(CuTest *t)
{
    memset(&Model, 0, sizeof(Model));
    Model.trims[0].value[0] = 10;
    for(int i = 0; i < 100; i++) {
        Model.trims[0].step = i;
        CuAssertIntEquals(t, 100 * i, MIXER_GetTrimValue(0));
    }
    for(int i = 100; i <= 190; i++) {
        Model.trims[0].step = i;
        CuAssertIntEquals(t, 1000 * (i- 90), MIXER_GetTrimValue(0));
    }
}

void TestUpdateTrim(CuTest *t)
{
    int neg = 4;
    int pos = 5;
    memset(&Model, 0, sizeof(Model));
    Model.trims[0].neg = neg;
    Model.trims[0].pos = pos;
    Model.trims[0].step = 1;

    //Button Up
    Model.trims[0].value[0] = 1;
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), 0, NULL);
    CuAssertIntEquals(t, 2, Model.trims[0].value[0]);

    //Button Down
    Model.trims[0].value[0] = 0;
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), 0, NULL);
    CuAssertIntEquals(t, -1, Model.trims[0].value[0]);

    //Button Long Up
    Model.trims[0].value[0] = 1;
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_LONGPRESS, NULL);
    CuAssertIntEquals(t, 10, Model.trims[0].value[0]);
    CuAssertIntEquals(t, 0, TEST_Button_InterruptLongPress());

    //Button Long Down
    Model.trims[0].value[0] = 0;
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_LONGPRESS, NULL);
    CuAssertIntEquals(t, -9, Model.trims[0].value[0]);
    CuAssertIntEquals(t, 0, TEST_Button_InterruptLongPress());

    //Long crossing zero down
    Model.trims[0].value[0] = 5;
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_LONGPRESS, NULL);
    CuAssertIntEquals(t, 0, Model.trims[0].value[0]);
    CuAssertIntEquals(t, 1, TEST_Button_InterruptLongPress());

    //Long crossing zero up 
    Model.trims[0].value[0] = -5;
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_LONGPRESS, NULL);
    CuAssertIntEquals(t, 0, Model.trims[0].value[0]);
    CuAssertIntEquals(t, 1, TEST_Button_InterruptLongPress());

    //Button max up
    Model.trims[0].value[0] = 99;
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_LONGPRESS, NULL);
    CuAssertIntEquals(t, 100, Model.trims[0].value[0]);
    CuAssertIntEquals(t, 1, TEST_Button_InterruptLongPress());

    //Button max down
    Model.trims[0].value[0] = -99;
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_LONGPRESS, NULL);
    CuAssertIntEquals(t, -100, Model.trims[0].value[0]);
    CuAssertIntEquals(t, 1, TEST_Button_InterruptLongPress());
}

void TestTrimAsSwitch(CuTest *t) 
{
    int neg = 4;
    int pos = 5;
    memset(&Model, 0, sizeof(Model));
    Model.trims[0].neg = neg;
    Model.trims[0].pos = pos;

    //ON/OFF
    Model.trims[0].step = TRIM_ONOFF;
    //Button Up
    Model.trims[0].value[0] = 0;
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_PRESS, NULL);
    CuAssertIntEquals(t, 100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_RELEASE, NULL);
    CuAssertIntEquals(t, 100, Model.trims[0].value[0]);

    //Button Down
    Model.trims[0].value[0] = 0;
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_PRESS, NULL);
    CuAssertIntEquals(t, -100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_RELEASE, NULL);
    CuAssertIntEquals(t, -100, Model.trims[0].value[0]);


    //TOGGLE
    Model.trims[0].step = TRIM_TOGGLE;
    Model.trims[0].value[0] = 0;
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_PRESS, NULL);
    CuAssertIntEquals(t, -100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_RELEASE, NULL);
    CuAssertIntEquals(t, -100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_PRESS, NULL);
    CuAssertIntEquals(t, 100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_RELEASE, NULL);
    CuAssertIntEquals(t, 100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_PRESS, NULL);
    CuAssertIntEquals(t, -100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_RELEASE, NULL);
    CuAssertIntEquals(t, -100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_PRESS, NULL);
    CuAssertIntEquals(t, 100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_RELEASE, NULL);
    CuAssertIntEquals(t, 100, Model.trims[0].value[0]);

    //Momentary
    Model.trims[0].step = TRIM_MOMENTARY;
    Model.trims[0].value[0] = 0;
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_PRESS, NULL);
    CuAssertIntEquals(t, 100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_RELEASE, NULL);
    CuAssertIntEquals(t, -100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_PRESS, NULL);
    CuAssertIntEquals(t, 100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_RELEASE, NULL);
    CuAssertIntEquals(t, -100, Model.trims[0].value[0]);

    //3Pos
    Model.trims[0].step = TRIM_3POS;
    Model.trims[0].value[0] = 0;
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_PRESS, NULL);
    CuAssertIntEquals(t, 100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(pos), BUTTON_RELEASE, NULL);
    CuAssertIntEquals(t, 0, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_PRESS, NULL);
    CuAssertIntEquals(t, -100, Model.trims[0].value[0]);
    MIXER_UpdateTrim(CHAN_ButtonMask(neg), BUTTON_RELEASE, NULL);
    CuAssertIntEquals(t, 0, Model.trims[0].value[0]);
}
