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
#include "protocol/interface.h"
#include "mixer.h"
#include "misc.h"
#include "gui/gui.h"
#include "buttons.h"
#include "timer.h"
#include "config/model.h"
#include "config/tx.h"

void Init();
void Banner();
void EventLoop();

void TOUCH_Handler(); // temporarily in main()

u32 next_redraw=0;

int main() {
    
    Init();
    Banner();

    if(PWR_CheckPowerSwitch()) PWR_Shutdown();

    LCD_Clear(0x0000);

    LCD_SetFont(1);
    LCD_SetFontColor(0xffff);

    u32 buttons = ScanButtons();
    if(CHAN_ButtonIsPressed(buttons, BUT_ENTER)) {
        LCD_PrintStringXY(10, 10, "USB Storage mode, press 'ENT' to exit.");
        USB_Connect();
    }
    
    while(!FS_Mount()) {
        LCD_PrintStringXY(10, 10, "Filesystem could not be mounted. Create the FS via USB, then press 'ENT' to exit.");
        USB_Connect();
    }

    CONFIG_LoadTx();
    CONFIG_ReadDisplay();
    CONFIG_ReadModel(CONFIG_GetCurrentModel());

    BACKLIGHT_Brightness(Transmitter.brightness);
    LCD_SetFont(DEFAULT_FONT.font);
    LCD_SetFontColor(DEFAULT_FONT.font_color);

    MUSIC_Play(MUSIC_STARTUP);
    GUI_HandleButtons(1);

    MIXER_Init();
    PAGE_Init();
    
    CLOCK_StartWatchdog();
    
    next_redraw = CLOCK_getms()+100;
    
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
    SPI_FlashBlockWriteEnable(1); //Enable writing to all banks of SPIFlash
}

void Banner()
{
    u8 Power = CYRF_MaxPower();
    u8 mfgdata[6];
    u8 tmp[12];
    ModelName(tmp, 12);
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

    if(PWR_CheckPowerSwitch()) {
        CONFIG_SaveModelIfNeeded();
        CONFIG_SaveTxIfNeeded();
        PWR_Shutdown();
        
    }

    MIXER_CalcChannels();

    BUTTON_Handler();
    TOUCH_Handler();

    PAGE_Event();

    if (CLOCK_getms() > next_redraw) {
        if (PROTOCOL_WaitingForSafe())
            PAGE_ShowSafetyDialog();
        TIMER_Update();
        GUI_RefreshScreen();
        next_redraw = CLOCK_getms() + 100;
    }
}

void TOUCH_Handler() {
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
