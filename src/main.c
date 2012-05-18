
#include "target.h"
#include "misc.h"
#include "gui/gui.h"

void event_loop(void *);
void channel_scanner();
void dump_bootloader();
extern void start_event_loop();

#define PAGEINIT  PAGE_TestInit
#define PAGEEVENT PAGE_TestEvent

//#define PAGEINIT  PAGE_MixerInit
//#define PAGEEVENT PAGE_MixerEvent

int main() {
    PWR_Init();
    CLOCK_Init();
    Initialize_ButtonMatrix();
    Delay(0x2710);

    LCD_Init();
    CHAN_Init();

    SPIFlash_Init();
    CYRF_Initialize();
    UART_Initialize();
    SPITouch_Init();
    SOUND_Init();
    FS_Mount();
    SPI_FlashBlockWriteEnable(1); //Enable writing to all banks of SPIFlash
    SignOn();
    LCD_Clear(0x0000);
    LCD_SetFont(6);
    LCD_SetFontColor(0xffff);

#if 0
    printf("Showing display\n");
    LCD_PrintStringXY(10, 10, "Hello");
    while(1) {
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
    }
#endif
#ifdef BL_DUMP
    dump_bootloader();
#endif
#if SCANNER
    channel_scanner();
#endif
#ifdef STATUS_SCREEN
    //PAGE_InitMixer(0);
    PAGEINIT(0);
#ifdef HAS_EVENT_LOOP
    start_event_loop();
#else
    while(1)
        event_loop(NULL);
#endif
    return 0;
#endif
}

void event_loop(void *param)
{
    /* Some needed variables */
    static u32 last_buttons = 0;
    static u8 touch_down = 0;
    static u32 last_redraw = 0;
    (void)(param);

    int i;
    if(PWR_CheckPowerSwitch())
        PWR_Shutdown();
    {
        u32 buttons = ScanButtons();

        /* GUI Handling
         * We beed to handle screen redraws here
         * */
        if(buttons != last_buttons) {
            char buttonstring[33];
            last_buttons = buttons;
            if((buttons & 0x01) == 0)
                LCD_CalibrateTouch();
            if((buttons & 0x02) == 0)
                USB_Connect();
            for(i = 0; i < 32; i++)
                buttonstring[i] = (buttons & (1 << i)) ? '0' : '1';
            buttonstring[32] = 0;
            printf("Buttons: %s\n",buttonstring);
        }
    }
    if(SPITouch_IRQ()) {
        struct touch t = SPITouch_GetCoords();
        printf("x : %4d y : %4d\n", t.x, t.y);
        if (! touch_down) {
            GUI_CheckTouch(t);
            touch_down = 1;
        }
    } else {
        touch_down = 0;
    }
    MIX_CalcChannels();

    PAGEEVENT();

    if (CLOCK_getms() > last_redraw + 100) {
        /* Redraw everything */
        GUI_RefreshScreen();
        last_redraw = CLOCK_getms();
    }
}

#if SCANNER
void channel_scanner()
{
#define NUM_CHANNELS    0x50

    u32 i,j,k;
    u8 dpbuffer[16];
    u8 channelnoise[NUM_CHANNELS];
    u8 channel = 0x04;

    CYRF_ConfigRxTx(1);
    CYRF_ConfigCRCSeed(0);
    CYRF_ConfigSOPCode(0);

    while(1)
    {
        if(PWR_CheckPowerSwitch())
        PWR_Shutdown();

        CYRF_ConfigRFChannel(channel);
        CYRF_StartReceive();
        Delay(10);

        CYRF_ReadDataPacket(dpbuffer);
        channelnoise[channel] = CYRF_ReadRSSI(1);

        printf("%02X : %d\n",channel,channelnoise[channel]);

        channel++;
        if(channel == NUM_CHANNELS)
        {
            channel = 0x04;
            LCD_Clear(0x0000);

            for(i=4;i<NUM_CHANNELS;i++)
            {
                LCD_SetDrawArea(30 + (3*i), 30, 31 + (3*i), 190);
                LCD_DrawStart();
                for(k=0;k<16; k++)
                {
                    for(j=0; j<2; j++)
                    {
                        if(k < (15 - channelnoise[i]))
                        {
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                        }
                        else
                        {
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                        }
                    }
                }
                LCD_DrawStop();
            }
        }
    }
}
#endif

#ifdef BL_DUMP
void dump_bootloader()
{
    LCD_PrintStringXY(40, 10, "Dumping");

    printf("Erase...\n");

    SPIFlash_EraseSector(0x2000);
    SPIFlash_EraseSector(0x3000);
    SPIFlash_EraseSector(0x4000);
    SPIFlash_EraseSector(0x5000);

    printf("Pgm 2\n");
    SPIFlash_WriteBytes(0x2000, 0x1000, (u8*)0x08000000);
    printf("Pgm 3\n");
    SPIFlash_WriteBytes(0x3000, 0x1000, (u8*)0x08001000);
    printf("Pgm 4\n");
    SPIFlash_WriteBytes(0x4000, 0x1000, (u8*)0x08002000);
    printf("Pgm 5\n");
    SPIFlash_WriteBytes(0x5000, 0x1000, (u8*)0x08003000);
    printf("Done\n");

    LCD_Clear(0x0000);
    LCD_PrintStringXY(40, 10, "Done");

    while(1)
    {
        if(PWR_CheckPowerSwitch())
        PWR_Shutdown();
    }
}
#endif
