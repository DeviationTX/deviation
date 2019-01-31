#include "CuTest.h"

extern void AssertScreenshot(CuTest* t, const char* filename);

void TestFontLoad(CuTest* t)
{
    memset(FontNames, 0, sizeof(FontNames));
    FONT_GetFromString("font1");
    CuAssertIntEquals(t, FONT_GetFromString("font1"), 1);
    FONT_GetFromString("font2");
    CuAssertIntEquals(t, FONT_GetFromString("font2"), 2);

    CuAssertIntEquals(t, FONT_GetFromString("font3"), 3);
}

void TestFontRender(CuTest* t)
{
    memset(FontNames, 0, sizeof(FontNames));
    int idx = FONT_GetFromString("15normal");
    LCD_Clear(0);
    LCD_SetFont(idx);
    LCD_SetFontColor(0xF0);
    LCD_PrintStringXY(0, 0, "the quick brown fox jumps over the lazy dog");
    LCD_PrintStringXY(5, 20, "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG");
    LCD_PrintStringXY(15, 40, "0123456789)!@#$%^&*(");

    AssertScreenshot(t, "font");
}
