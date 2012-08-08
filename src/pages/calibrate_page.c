/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "target.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/tx.h"
#include "config/model.h"
#include "icons.h"

#define cp (pagemem.u.calibrate_page)
enum calibType {
    CALIB_NONE,
    CALIB_TOUCH,
    CALIB_TOUCH_TEST,
    CALIB_STICK,
    CALIB_STICK_TEST,
};

#define XCOORD 20
#define YCOORD 20
static void get_coords(struct touch *t)
{
    /* Wait for button press */
    while(! SPITouch_IRQ()) {
        CLOCK_ResetWatchdog();
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
    }
    *t = SPITouch_GetCoords();
    /* Wait for button releasde */
    while(SPITouch_IRQ()) {
        CLOCK_ResetWatchdog();
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
    }
}

static void draw_target(u16 x, u16 y)
{
    LCD_DrawFastHLine(x - 5, y, 11, SMALLBOX_FONT.font_color);
    LCD_DrawFastVLine(x, y - 5, 11, SMALLBOX_FONT.font_color);
}

static void calibrate_sticks(void)
{
    int i;
    struct touch t1;
    CLOCK_StopTimer();

    while(SPITouch_IRQ()) {
        CLOCK_ResetWatchdog();
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
    }
    LCD_Clear(0x0000);
    LCD_SetFontColor(0xFFFF);
    LCD_PrintStringXY(40, 10, "Center all sticks and touch the screen");
    for (i = 0; i < 4; i++) {
        Transmitter.calibration[i].max = 0x0000;
        Transmitter.calibration[i].min = 0xFFFF;
    }
    get_coords(&t1);
    for (i = 0; i < 4; i++) {
        s32 value = CHAN_ReadRawInput(i + 1);
        Transmitter.calibration[i].zero = value;
    }
    LCD_Clear(0x0000);
    LCD_PrintStringXY(40, 10, "Move each stick to full max\nand min values then back to center.\nTouch the screen when finished");
    while(! SPITouch_IRQ()) {
        CLOCK_ResetWatchdog();
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
        for (i = 0; i < 4; i++) {
            s32 value = CHAN_ReadRawInput(i + 1);
            if (value > Transmitter.calibration[i].max)
                Transmitter.calibration[i].max = value;
            else if (value < Transmitter.calibration[i].min)
                Transmitter.calibration[i].min = value;
        }
    }
    while(SPITouch_IRQ()) {
        CLOCK_ResetWatchdog();
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
    }
    for (i = 0; i < 4; i++) {
        printf("Input %d: Max: %d Min: %d Zero: %d\n", i+1, Transmitter.calibration[i].max, Transmitter.calibration[i].min, Transmitter.calibration[i].zero);
    }
    PROTOCOL_Init(Model.protocol);
    PAGE_CalibrateInit(0);
}

static void calibrate_touch(void)
{
    if (cp.state == 0 || cp.state == 3) {
        if (GUI_ObjectNeedsRedraw(cp.textbox))
            return;
        draw_target(cp.state ? 320 - XCOORD : XCOORD , cp.state ? 240 - YCOORD : YCOORD + 32);
        cp.state++;
    } else if (cp.state == 1 || cp.state == 4) {
        if (SPITouch_IRQ()) {
            cp.coords = SPITouch_GetCoords();
            cp.state++;
        }
    } else if (cp.state == 2) {
        if (! SPITouch_IRQ()) {
            cp.coords1 = cp.coords;
            GUI_RemoveObj(cp.textbox);
            cp.textbox = GUI_CreateLabelBox(320 - XCOORD - 5, 240 - YCOORD - 5,
                                            11, 11, &SMALLBOX_FONT, NULL, NULL, "");
            GUI_Redraw(cp.textbox1);
            cp.state = 3;
        } else {
            cp.coords = SPITouch_GetCoords();
        }
    } else if (cp.state == 5) {
        if (! SPITouch_IRQ()) {
            s32 xscale, yscale;
            s32 xoff, yoff;
            printf("T1:(%d, %d)\n", cp.coords1.x, cp.coords1.y);
            printf("T2:(%d, %d)\n", cp.coords.x, cp.coords.y);
            xscale = cp.coords.x - cp.coords1.x;
            xscale = (320 - 2 * XCOORD) * 0x10000 / xscale;
            yscale = cp.coords.y - cp.coords1.y;
            yscale = (240 - 32 - 2 * YCOORD) * 0x10000 / yscale;
            xoff = XCOORD - cp.coords1.x * xscale / 0x10000;
            yoff = YCOORD + 32 - cp.coords1.y * yscale / 0x10000;
            printf("Debug: scale(%d, %d) offset(%d, %d)\n", (int)xscale, (int)yscale, (int)xoff, (int)yoff);
            SPITouch_Calibrate(xscale, yscale, xoff, yoff);
            PAGE_CalibrateInit(0);
        } else {
            cp.coords = SPITouch_GetCoords();
        }
    }
}

static const char *calibratestr_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    return (long)data & 1 ? "Calibrate" : "Test";
}
const char *coords_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    sprintf(cp.tmpstr, "%d*%d-%d-%d", cp.coords.x, cp.coords.y, cp.coords.z1, cp.coords.z2);
    return cp.tmpstr;
}

const char *show_msg_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    sprintf(cp.tmpstr, "Touch taget %d", cp.state < 3 ? 1 : 2);
    return cp.tmpstr;
}

static void okcancel_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    PAGE_CalibrateInit(0);
}

static void press_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    cp.enable = (long)data;
    if (cp.enable == CALIB_TOUCH) {
        PAGE_RemoveAllObjects();
        PAGE_SetModal(1);
        //PAGE_CreateOkButton(264, 4, okcancel_cb);
        GUI_CreateIcon(0, 0, &icons[ICON_EXIT], okcancel_cb, (void *)0);
        GUI_CreateLabel(40, 10, NULL, TITLE_FONT, "Touch Calibrate");
        cp.textbox = GUI_CreateLabelBox(XCOORD - 5, YCOORD + 32 - 5, 11, 11, &SMALLBOX_FONT, NULL, NULL, "");
        cp.textbox1 = GUI_CreateLabelBox(130, 110, 0, 0, &DEFAULT_FONT, show_msg_cb, NULL, NULL);
        memset(&cp.coords, 0, sizeof(cp.coords));
        cp.state = 0;
    }
    if (cp.enable == CALIB_TOUCH_TEST) {
        PAGE_RemoveAllObjects();
        PAGE_SetModal(1);
        //PAGE_CreateOkButton(264, 4, okcancel_cb);
        GUI_CreateIcon(0, 0, &icons[ICON_EXIT], okcancel_cb, (void *)0);
        GUI_CreateLabel(40, 10, NULL, TITLE_FONT, "Touch Test");
        cp.textbox = GUI_CreateLabelBox(60, 110, 150, 25, &SMALLBOX_FONT, coords_cb, NULL, NULL);
        memset(&cp.coords, 0, sizeof(cp.coords));
    }
}
void PAGE_CalibrateInit(int page)
{
    (void)page;
    cp.enable = CALIB_NONE;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader("Calibrate");

    GUI_CreateLabelBox(20, 106, 0, 0, &DEFAULT_FONT, NULL, NULL, "Screen");
    GUI_CreateButton(80, 100, BUTTON_96, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_TOUCH);
    GUI_CreateButton(180, 100, BUTTON_96, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_TOUCH_TEST);
    GUI_CreateLabelBox(20, 146, 0, 0, &DEFAULT_FONT, NULL, NULL, "Sticks");
    GUI_CreateButton(80, 140, BUTTON_96, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_STICK);
    // GUI_CreateButton(180, 140, BUTTON_96, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_STICK_TEST);
}


void PAGE_CalibrateEvent()
{
    switch(cp.enable) {
    case CALIB_TOUCH: {
        calibrate_touch();
        break;
    }
    case CALIB_TOUCH_TEST: {
        struct touch t;
        if (SPITouch_IRQ()) {
            t = SPITouch_GetCoords();
            if (memcmp(&t, &cp.coords, sizeof(t)) != 0)
                cp.coords = t;
                GUI_Redraw(cp.textbox);
        }
        break;
    }
    case CALIB_STICK:
        calibrate_sticks();
        break;
    case CALIB_STICK_TEST:
    default: break;
    }
}

