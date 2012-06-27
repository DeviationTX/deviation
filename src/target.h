#ifndef _TARGET_H_
#define _TARGET_H_
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
//Load target-specific include
#include TARGET_H

struct Calibration {
    u16 max;
    u16 min;
    u16 zero;
};

struct Transmitter {
    struct Calibration calibration[NUM_INPUTS];
};
extern struct Transmitter Transmitter;
extern s16 Channels[NUM_CHANNELS];

/* General functions */
void SignOn();

/* Display Functions */
void LCD_Init();

enum DrawDir {
    DRAW_NWSE,
    DRAW_SWNE,
};

    /* Primitives */
void LCD_DrawPixel(unsigned int color);
void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color);
void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir);
void LCD_DrawStop(void);
void LCD_Clear(unsigned int color);
    /* Strings */
void LCD_PrintCharXY(unsigned int x, unsigned int y, const char c);
void LCD_PrintChar(const char c);
void LCD_PrintStringXY(unsigned int x, unsigned int y, const char *str);
void LCD_PrintString(const char *str);
void LCD_SetXY(unsigned int x, unsigned int y);
void LCD_GetStringDimensions(const u8 *str, u16 *width, u16 *height);
u8 LCD_SetFont(unsigned int idx);
u8  LCD_GetFont();
void LCD_SetFontColor(u16 color);
    /* Graphics */
void LCD_DrawCircle(u16 x0, u16 y0, u16 r, u16 color);
void LCD_FillCircle(u16 x0, u16 y0, u16 r, u16 color);
void LCD_DrawLine(u16 x0, u16 y0, u16 x1, u16 y1, u16 color);
void LCD_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void LCD_DrawFastHLine(u16 x, u16 y, u16 w, u16 color);
void LCD_DrawDashedVLine(int16_t x, int16_t y, int16_t h, int16_t space, uint16_t color);
void LCD_DrawDashedHLine(int16_t x, int16_t y, int16_t w, int16_t space, uint16_t color);
void LCD_DrawRect(u16 x, u16 y, u16 w, u16 h, u16 color);
void LCD_FillRect(u16 x, u16 y, u16 w, u16 h, u16 color);
void LCD_DrawRoundRect(u16 x, u16 y, u16 w, u16 h, u16 r, u16 color);
void LCD_FillRoundRect(u16 x, u16 y, u16 w, u16 h, u16 r, u16 color);
void LCD_DrawTriangle(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void LCD_FillTriangle(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void LCD_DrawWindowedImageFromFile(u16 x, u16 y, const char *file, s16 w, s16 h, u16 x_off, u16 y_off);
void LCD_DrawImageFromFile(u16 x, u16 y, const char *file);
u8 LCD_ImageIsTransparent(const char *file);
u8 LCD_ImageDimensions(const char *file, u16 *w, u16 *h);
void LCD_CalibrateTouch(void);

/* Touchscreen */
struct touch {
    u16 x;
    u16 y;
    u16 z1;
    u16 z2;
};
void SPITouch_Init();
struct touch SPITouch_GetCoords();
int SPITouch_IRQ();
void SPITouch_Calibrate(s32 xscale, s32 yscale, s32 xoff, s32 yoff);

/* Buttons and switches */
void Initialize_ButtonMatrix();
u32 ScanButtons();

/* Power functions */
void PWR_Init(void);
u16  PWR_ReadVoltage(void);
int  PWR_CheckPowerSwitch();
void PWR_Shutdown();

/* Clock functions */
void CLOCK_Init(void);
u32 CLOCK_getms(void);
void CLOCK_StartTimer(u16 us, u16 (*cb)(void));
void CLOCK_StopTimer();

/* Sticks */
void CHAN_Init();
s16  CHAN_ReadInput(int channel);

/* SPI Flash */
void SPIFlash_Init();
u32  SPIFlash_ReadID();
void SPIFlash_EraseSector(u32 sectorAddress);
void SPIFlash_BulkErase();
void SPIFlash_WriteBytes(u32 writeAddress, u32 length, u8 * buffer);
void SPIFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer);
void SPI_FlashBlockWriteEnable(u8 enable);

/* SPI CYRF6936 */
void CYRF_Initialize();
void CYRF_GetMfgData(u8 data[]);

void CYRF_ConfigRxTx(u32 TxRx);
void CYRF_ConfigRFChannel(u8 ch);
void CYRF_ConfigCRCSeed(u8 crc);
void CYRF_StartReceive();
void CYRF_ConfigSOPCode(u32 idx);
u8 CYRF_ReadRSSI(u32 dodummyread);
void CYRF_ReadDataPacket(u8 dpbuffer[]); 
void CYRF_WriteDataPacket(u8 dpbuffer[]); 
void CYRF_WriteRegister(u8 address, u8 data);
u8 CYRF_ReadRegister(u8 address);

/* Sound */
void SOUND_Init();
void SOUND_SetPeriod(u16 period);
void SOUND_Enable();
void SOUND_Disable();

/* UART & Debug */
void UART_Initialize();

/* USB*/
void USB_Enable(u8 use_interrupt);
void USB_Disable();
void USB_HandleISR();
void USB_Connect();

/* Filesystem */
int FS_Mount();
void FS_Unmount();

/* Mixer functions */
void MIX_CalcChannels();
void MIX_UpdateTrim(u32 buttons);

/* GUI Pages */
void PAGE_Init();
void PAGE_Change(int dir);
void PAGE_Event();

/* Protocol */
enum Protocols {
    PROTOCOL_FLYSKY,
    PROTOCOL_DEVO,
};
void PROTOCOL_Init(enum Protocols p);
void PROTOCOL_DeInit();

/* Abstract bootloader access */
enum {
    BL_ID = 0,
};
u8 *BOOTLOADER_Read(int idx);
#endif
