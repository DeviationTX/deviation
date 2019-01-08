#include "CuTest.h"

extern void AssertScreenshot(CuTest* t, const char* filename);
extern u8 FONT_GetFromString(const char *);

static void InitializeFont()
{
    int idx = FONT_GetFromString("15normal");
    LCD_Clear(0);
    DEFAULT_FONT.font = idx;
    DEFAULT_FONT.font_color = 0xff;
}

void TestLabel(CuTest* t)
{
    guiLabel_t label;
    InitializeFont();

    GUI_CreateLabelBox(&label, 10, 10, LCD_WIDTH, 15, &DEFAULT_FONT,
        NULL, NULL, "TestLabel");

    GUI_DrawObject(&label);
    AssertScreenshot(t, "label.bmp");
}
