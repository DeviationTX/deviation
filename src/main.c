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
#include "autodimmer.h"
#include "music.h"
#include "config/model.h"
#include "config/tx.h"
#include "config/display.h"
#include "rtc.h"
#include "extended_audio.h"

void Init();
void Banner();
void EventLoop();
volatile u8 priority_ready;

void TOUCH_Handler(); // temporarily in main()
void VIDEO_Update();
void PAGE_Test();

#ifndef DUMP_BOOTLOADER
int main() {

    Init();
#ifndef MODULAR
    //Banner();
#endif
    if(PWR_CheckPowerSwitch()) PWR_Shutdown();

    LCD_Clear(0x0000);
#ifdef TEST_ADC
    ADC_ScanChannels(); while(1);
#endif
    u32 buttons = ScanButtons();
    if(CHAN_ButtonIsPressed(buttons, BUT_ENTER) || !FS_Mount(NULL, NULL)) {
        LCD_DrawUSBLogo(LCD_WIDTH, LCD_HEIGHT);
        USB_Connect();
        LCD_Clear(0x0000);
        FS_Mount(NULL, NULL);
    }

    CONFIG_LoadTx();
    SPI_ProtoInit();
    CONFIG_ReadDisplay();
    CONFIG_ReadModel(CONFIG_GetCurrentModel());
    CONFIG_ReadLang(Transmitter.language);

    BACKLIGHT_Brightness(Transmitter.backlight);
    LCD_Contrast(Transmitter.contrast);
    LCD_SetFont(DEFAULT_FONT.font);
    LCD_SetFontColor(DEFAULT_FONT.font_color);

#if !HAS_EXTENDED_AUDIO
    // If Extended Audio is present, move startup msg to Splash page to allow additional audio hardware initialization time
    MUSIC_Play(MUSIC_STARTUP);
#endif
    GUI_HandleButtons(1);

    MIXER_Init();
    PAGE_Init();

    CLOCK_StartWatchdog();

#if HAS_DATALOG
    DATALOG_Init();
#endif

    priority_ready = 0;
    CLOCK_SetMsecCallback(LOW_PRIORITY, LOW_PRIORITY_MSEC);
    CLOCK_SetMsecCallback(MEDIUM_PRIORITY, MEDIUM_PRIORITY_MSEC);

    // We need to wait until we've actually measured the ADC before proceeding
    while(! (priority_ready & (1 << LOW_PRIORITY)))
        ;

    //Only do this after we've initialized all channel data so the saftey works
    PROTOCOL_InitModules();
    GUI_DrawScreen();

#ifdef HAS_EVENT_LOOP
    start_event_loop();
#else
    while(1) {
        if(priority_ready) {
            EventLoop();
        }
        //This does not appear to have any impact on power
        //and has been disabled in common/devo/power.c
        //but it helps a huge amount for the emulator
        PWR_Sleep();
    }
#endif
}
#else //DUMP_BOOTLOADER
#define SPI_BOOTLOADER 1
int main() {
    PWR_Init();
    CLOCK_Init();
    UART_Initialize();
    if(PWR_CheckPowerSwitch()) PWR_Shutdown();
#if SPI_BOOTLOADER
    Initialize_ButtonMatrix();
    SPIFlash_Init(); //This must come before LCD_Init() for 7e
    SPI_FlashBlockWriteEnable(1); //Enable writing to all banks of SPIFlash
    LCD_Init();
    LCD_Clear(0x0000);
    BACKLIGHT_Init();
    BACKLIGHT_Brightness(5);
    LCD_SetFont(0);
    LCD_SetFontColor(0xffff);
    dump_bootloader(0);
#else
    dump_bootloader(1);
#endif
}
#endif //DUMP_BOOTLOADER

void Init() {
    PWR_Init();
    CLOCK_Init();
    UART_Initialize();
    printf("Start\n");
    Initialize_ButtonMatrix();
    SPIFlash_Init(); //This must come before LCD_Init() for 7e

    LCD_Init();
    CHAN_Init();

    SPITouch_Init();
    SOUND_Init();
    BACKLIGHT_Init();
    BACKLIGHT_Brightness(1);
    AUTODIMMER_Init();
    SPI_FlashBlockWriteEnable(1); //Enable writing to all banks of SPIFlash

    PPMin_TIM_Init();
#ifdef MODULAR
    //Force protocol to none to initialize RAM
    Model.protocol = PROTOCOL_NONE;
    PROTOCOL_Init(1);
#endif
#if HAS_RTC
    RTC_Init();        // Watchdog must be running in case something goes wrong (e.g no crystal)
#endif
}

void Banner()
{
    CYRF_Reset();
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

void medium_priority_cb()
{
#ifdef TIMING_DEBUG
    debug_timing(3, 0);
#endif
    MIXER_CalcChannels();
#ifdef TIMING_DEBUG
    debug_timing(3, 1);
#endif
}

void EventLoop()
{
    CLOCK_ResetWatchdog();
    unsigned int time;

#ifdef HEAP_DEBUG
    static int heap = 0;
    int h = _sbrk_r(NULL, 0);
    if(h > heap) {
        printf("heap: %x\n", h);
        heap = h;
    }
#endif
#ifdef TIMING_DEBUG
    debug_timing(0, 0);
#endif
    priority_ready &= ~(1 << MEDIUM_PRIORITY);
    if(PWR_CheckPowerSwitch()) {
        if(! (BATTERY_Check() & BATTERY_CRITICAL)) {
            PAGE_Test();
            CONFIG_SaveModelIfNeeded();
            CONFIG_SaveTxIfNeeded();
        }
    	if(Transmitter.music_shutdown) {
	    MUSIC_Play(MUSIC_SHUTDOWN);
            // We wait ~1sec for shutdown music finished
            time = CLOCK_getms()+700;
            while(CLOCK_getms()<time);
	}

        PWR_Shutdown();
    }
    BUTTON_Handler();
    TOUCH_Handler();
    INPUT_CheckChanges();

    if (priority_ready & (1 << LOW_PRIORITY)) {
        priority_ready  &= ~(1 << LOW_PRIORITY);
        PAGE_Event();
        PROTOCOL_CheckDialogs();
        TIMER_Update();
        TELEMETRY_Alarm();
        BATTERY_Check();
        AUTODIMMER_Update();
#if HAS_DATALOG
        DATALOG_Update();
#endif
#if HAS_VIDEO
        VIDEO_Update();
#endif
#if HAS_EXTENDED_AUDIO
        AUDIO_CheckQueue();
#endif
        GUI_RefreshScreen();
    }
#ifdef TIMING_DEBUG
    debug_timing(0, 1);
#endif
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
        AUTODIMMER_Check();
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

#if HAS_VIDEO
void VIDEO_Update()
{
    static u8 video_enable = 0;
    static u8 video_standard_in_use = 0xFE; //unknown
    //FIXME This is just like DATALOG_IsEnabled
    int enabled = MIXER_SourceAsBoolean(Model.videosrc);

    if (enabled != video_enable) {
        VIDEO_Enable(enabled);
        video_enable = enabled;
        if (enabled) {
            VIDEO_SetChannel(Model.videoch);
            VIDEO_Contrast(Model.video_contrast);
            VIDEO_Brightness(Model.video_brightness);
        }
    }
    if(video_enable) {
        u8 video_standard_current = VIDEO_GetStandard();
        if((video_standard_current > 0) &&
           (video_standard_current < 7) &&
           (video_standard_current != video_standard_in_use)) {
            video_standard_in_use = video_standard_current;
            VIDEO_SetStandard(video_standard_current);
        }
    }
}
#endif //HAS_VIDEO

#ifdef TIMING_DEBUG
void debug_timing(u32 type, int startend)
{
    static u32 last_time[2][100];
    static u32 loop_time[4][101];
    static u32 loop_pos[4] = {-1, -1, -1, -1};
    static u32 max_last[2];
    static u32 max_loop[4];
    static int save_priority;

    if (type == 0) {
        if (! startend)
            save_priority = priority_ready;
        if (save_priority & (1 << MEDIUM_PRIORITY))
            debug_timing(2, startend);
        if (save_priority & (1 << LOW_PRIORITY))
            debug_timing(1, startend);
        return;
    }
    type--;
    if (! startend) {
        u32 t = CLOCK_getms();
        loop_pos[type] = (loop_pos[type] + 1) % 100;
        if (type < 2) {
            last_time[type][loop_pos[type]] = t;
            if (t - last_time[type][(loop_pos[type] + 99) % 100] > max_last[type])
                max_last[type] = t - last_time[type][(loop_pos[type] + 99) % 100];
        }
        loop_time[type][100] = t;
    } else {
        loop_time[type][loop_pos[type]] = CLOCK_getms() - loop_time[type][100];
        if (loop_time[type][loop_pos[type]] > max_loop[type])
            max_loop[type] = loop_time[type][loop_pos[type]];
        if (type == 0 && loop_pos[0] == 99) {
            unsigned avg_loop[4] = {0, 0, 0, 0};
            unsigned avg_last[2] = {0, 0};
            for(int i = 0; i < 99; i++) {
                for(int t = 0; t < 2; t++) {
                    u32 delay = last_time[t][(i + loop_pos[t] + 2) % 100] - last_time[t][(i + loop_pos[t] + 1) % 100];
                    avg_last[t] += delay;
                }
                for(int t = 0; t < 4; t++)
                    avg_loop[t] += loop_time[t][i];
            }
            for(int t = 0; t < 4; t++)
                avg_loop[t] /= 99;
            avg_last[0] /= 99;
            avg_last[1] /= 99;
            printf("Avg: radio: %d mix: %d med: %d/%d low: %d/%d\n", avg_loop[3], avg_loop[2], avg_loop[1], avg_last[1], avg_loop[0], avg_last[0]);
            printf("Max: radio: %d mix: %d med: %d/%d low: %d/%d\n", max_loop[3], max_loop[2], max_loop[1], max_last[1], max_loop[0], max_last[0]);
            memset(max_loop, 0, sizeof(max_loop));
            max_last[0] = 0;
            max_last[1] = 0;
        }
    }
}
#endif

void debug_switches()
{
    s32 data[INP_LAST];
    for(int i = INP_HAS_CALIBRATION+1; i < INP_LAST; i++) {
        data[i] = CHAN_ReadRawInput(i);
    }
    while(1) {
        u32 changed = 0;
        for(int i = INP_HAS_CALIBRATION+1; i < INP_LAST; i++) {
            s32 val = CHAN_ReadRawInput(i);
            if (val != data[i]) {
                printf("%s=%d  ", INPUT_SourceName(tempstring, i), val);
                data[i] = val;
                changed = 1;
            }
        }
        if (changed) { printf("\n"); }
        if(PWR_CheckPowerSwitch()) PWR_Shutdown();
    }
}
void debug_buttons()
{
    u32 data = ScanButtons();
    while(1) {
        u32 val = ScanButtons();
        u32 delta = val ^ data;
        for(int i = 1; i < BUT_LAST; i++) {
            if(delta & (1 << (i-1))) {
                printf("%s(%s)  ", INPUT_ButtonName(i), (val &(1 << (i-1)))? "Down" : "Up");
            }
        }
        if (delta) {
            printf("\n");
            data = val;
        }
        if(PWR_CheckPowerSwitch()) PWR_Shutdown();
    }
}
