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

#include "common.h"
#include "protocol/interface.h"
#include "mixer.h"
#include "gui/gui.h"
#include "buttons.h"
#include "timer.h"
#include "music.h"
#include "config/model.h"
#include "config/tx.h"
#include "config/display.h"

void Init();
void Banner();
void EventLoop();

void TOUCH_Handler(); // temporarily in main()
u8 BATTERY_Check();

#define SCREEN_UPDATE_MSEC 100
#define CHAN_UPDATE_MSEC   5
u32 next_redraw=0;
u32 next_scan=0;

int main() {
    
    Init();
    Banner();

    if(PWR_CheckPowerSwitch()) PWR_Shutdown();

    LCD_Clear(0x0000);

    u32 buttons = ScanButtons();
    if(CHAN_ButtonIsPressed(buttons, BUT_ENTER) || !FS_Mount()) {
        LCD_DrawUSBLogo(LCD_WIDTH, LCD_HEIGHT);
        USB_Connect();
        LCD_Clear(0x0000);
        FS_Mount();
    }
    
    CONFIG_LoadTx();
    CONFIG_ReadDisplay();
    CONFIG_ReadModel(CONFIG_GetCurrentModel());
    CONFIG_ReadLang(Transmitter.language);

    BACKLIGHT_Brightness(Transmitter.brightness);
    LCD_Contrast(Transmitter.contrast);
    LCD_SetFont(DEFAULT_FONT.font);
    LCD_SetFontColor(DEFAULT_FONT.font_color);

    MUSIC_Play(MUSIC_STARTUP);
    GUI_HandleButtons(1);

    MIXER_Init();
    PAGE_Init();
    
    CLOCK_StartWatchdog();
    
    MIXER_CalcChannels();
    PROTOCOL_Init(0);

    next_redraw = CLOCK_getms()+SCREEN_UPDATE_MSEC;
    next_scan = CLOCK_getms()+CHAN_UPDATE_MSEC;
    
#ifdef HAS_EVENT_LOOP
    start_event_loop();
#else
    for(;;) EventLoop();
#endif
}

void Init() {
    PWR_Init();
    CLOCK_Init();
    UART_Initialize();
    Initialize_ButtonMatrix();

    LCD_Init();
    CHAN_Init();

    SPIFlash_Init();
    CYRF_Initialize();
#ifdef PROTO_HAS_A7105
    A7105_Initialize();
#endif
    SPITouch_Init();
    SOUND_Init();
    BACKLIGHT_Init();
    BACKLIGHT_Brightness(1);
    SPI_FlashBlockWriteEnable(1); //Enable writing to all banks of SPIFlash
}

void Banner()
{
    u8 Power = CYRF_MaxPower();
    u8 mfgdata[6];
    u8 tmp[12];
    TxName(tmp, 12);
    printf("\nDeviation\n");
    /* Check CPU type */

    printf("BootLoader    : '%s'\n",tmp);
    printf("Power         : '%s'\n",Power == CYRF_PWR_100MW ? "100mW" : "10mW" );
    printf("SPI Flash     : '%X'\n",(unsigned int)SPIFlash_ReadID());
    CYRF_GetMfgData(mfgdata);
    printf("CYRF Mfg Data : '%02X %02X %02X %02X %02X %02X'\n",
            mfgdata[0],
            mfgdata[1],
            mfgdata[2],
            mfgdata[3],
            mfgdata[4],
            mfgdata[5]);
    
}

void EventLoop()
{
    CLOCK_ResetWatchdog();

#ifdef HEAP_DEBUG
    static int heap = 0;
    int h = _sbrk_r(NULL, 0);
    if(h > heap) {
        printf("heap: %x\n", h);
        heap = h;
    }
#endif

    if(PWR_CheckPowerSwitch()) {
        if(! (BATTERY_Check() & 0x01)) {
            CONFIG_SaveModelIfNeeded();
            CONFIG_SaveTxIfNeeded();
        }
        PWR_Shutdown();
    }

    if (CLOCK_getms() > next_scan) {
        MIXER_CalcChannels();
        BUTTON_Handler();
        TOUCH_Handler();
        PAGE_Event();
        next_scan = CLOCK_getms() + CHAN_UPDATE_MSEC;
    }

    if (CLOCK_getms() > next_redraw) {
        PROTOCOL_CheckDialogs();
        TIMER_Update();
        TELEMETRY_Alarm();
        BATTERY_Check();
        GUI_RefreshScreen();
        next_redraw = CLOCK_getms() + SCREEN_UPDATE_MSEC;
    }
}

void TOUCH_Handler() {
    if(! HAS_TOUCH)
        return;
    u32 pen_down=0;

    static u32 pen_down_last=0;
    static u32 pen_down_long_at=0;

    struct touch t;
    
    if(SPITouch_IRQ()) {
        pen_down=1;
        t=SPITouch_GetCoords();
        if (! pen_down_last)
            pen_down_long_at=CLOCK_getms()+500;
    } else {
        pen_down=0;
    }
 
    if(pen_down && (!pen_down_last)) {
        GUI_CheckTouch(&t, 0);
    }
    
    if(!pen_down && pen_down_last) {
        GUI_TouchRelease();
    }
    
    if(pen_down && pen_down_last) {
        if(CLOCK_getms()>pen_down_long_at) {
            GUI_CheckTouch(&t, 1);
            pen_down_long_at += 100;
        }
    }
    pen_down_last=pen_down;
}

u8 BATTERY_Check()
{
    static u8 warned = 0;
    u16 battery = PWR_ReadVoltage();
    if (battery < Transmitter.batt_alarm && ! (warned & 0x02)) {
        MUSIC_Play(MUSIC_BATT_ALARM);
        warned |= 0x02;
    } else if (battery > Transmitter.batt_alarm + 200) {
        warned &= ~0x02;
    }
    if (battery < Transmitter.batt_critical && ! (warned & 0x01)) {
        CONFIG_SaveModelIfNeeded();
        CONFIG_SaveTxIfNeeded();
        SPI_FlashBlockWriteEnable(0); //Disable writing to all banks of SPIFlash
        warned |= 0x01;
        PAGE_ShowLowBattDialog();
    } else if (battery > Transmitter.batt_critical + 200) {
        warned &= ~0x01;
        SPI_FlashBlockWriteEnable(1); //Disable writing to all banks of SPIFlash
    }
    return warned;
}

