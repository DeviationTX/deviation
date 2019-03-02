#include "CuTest.h"

struct config_values {
    u8 u8_val;
    u16 u16_val;
    u32 u32_val;

    s8 s8_val;
    s16 s16_val;
    s32 s32_val;

    u16 color_val;
    u8 str_index_val;
    u8 font_val;

    u8 mixer;
} TestConfig;

enum {
    STR_NONE = 0,
    STR_CENTER,
    STR_LEFT,
    STR_RIGHT
};

static const char * const ALIGN_VAL[] = {
    [STR_CENTER]      = "center",
    [STR_LEFT]        = "left",
    [STR_RIGHT]       = "right",
    };

static const char *string_values(int i) {
    if (i == 0)
        return "standard";
    else
        return "advanced";
}

static const struct struct_map _secgeneral[] =
{
    {"u8val",         OFFSET(struct config_values, u8_val)},
    {"u16val",        OFFSET(struct config_values, u16_val)},
    {"u32val",        OFFSET(struct config_values, u32_val)},
    {"s8val",         OFFSETS(struct config_values, s8_val)},
    {"s16val",        OFFSETS(struct config_values, s16_val)},
    {"s32val",        OFFSETS(struct config_values, s32_val)},
    {"index",         OFFSET_STRLIST(struct config_values, str_index_val, ALIGN_VAL, ARRAYSIZE(ALIGN_VAL))},
    {"color",         OFFSET_COL(struct config_values, color_val)},
    {"font",          OFFSET_FON(struct config_values, font_val)},
    {"mixer",         OFFSET_STRCALL(struct config_values, mixer, string_values, 2)},
};

void TestConfigBasic(CuTest* t) {
    memset(&TestConfig, 0, sizeof(TestConfig));

    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "u8val", "5"));
    CuAssertIntEquals(t, 5, TestConfig.u8_val);
    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "s8val", "-5"));
    CuAssertIntEquals(t, -5, TestConfig.s8_val);

    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "u16val", "5000"));
    CuAssertIntEquals(t, 5000, TestConfig.u16_val);
    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "s16val", "-6005"));
    CuAssertIntEquals(t, -6005, TestConfig.s16_val);

    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "u32val", "755350"));
    CuAssertIntEquals(t, 755350, TestConfig.u32_val);
    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "s32val", "-655350"));
    CuAssertIntEquals(t, -655350, TestConfig.s32_val);

    CuAssertTrue(t, !assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "notfound", "0"));

    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "color", "000000"));
    CuAssertIntEquals(t, 0, TestConfig.color_val);
    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "color", "FEDCBA"));
    CuAssertIntEquals(t, 0xfef7, TestConfig.color_val);
}

void TestConfigStringList(CuTest* t)
{
    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "index", "center"));
    CuAssertIntEquals(t, STR_CENTER, TestConfig.str_index_val);

    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "index", "left"));
    CuAssertIntEquals(t, STR_LEFT, TestConfig.str_index_val);

    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "index", "right"));
    CuAssertIntEquals(t, STR_RIGHT, TestConfig.str_index_val);

    TestConfig.str_index_val = 254;
    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "index", "invalid"));
    CuAssertIntEquals(t, 254, TestConfig.str_index_val);
}

void TestConfigFont(CuTest* t)
{
    int fontindex = FONT_GetFromString("font1");

    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "font", "font1"));
    CuAssertIntEquals(t, fontindex, TestConfig.font_val);
}

void TestConfigStringCallback(CuTest* t)
{
    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "mixer", "standard"));
    CuAssertIntEquals(t, 0, TestConfig.mixer);

    CuAssertTrue(t, assign_int(&TestConfig, _secgeneral, ARRAYSIZE(_secgeneral), "mixer", "advanced"));
    CuAssertIntEquals(t, 1, TestConfig.mixer);
}
