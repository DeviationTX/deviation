#ifndef _TARGET_H_
#define _TARGET_H_
#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* General functions */
void SignOn();

/* Display Functions */
void LCD_Init();

    /* Primitives */
void LCD_DrawPixel(unsigned int color);
void LCD_DrawStart(void);
void LCD_DrawStop(void);
void LCD_SetDrawArea(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);
void LCD_Clear(unsigned int color);
    /* Strings */
void LCD_PrintCharXY(unsigned int x, unsigned int y, const char c);
void LCD_PrintChar(const char c);
void LCD_PrintStringXY(unsigned int x, unsigned int y, const char *str);
void LCD_PrintString(const char *str);
void LCD_SetXY(unsigned int x, unsigned int y);
void LCD_SetFont(unsigned int idx);

/* Buttons and switches */
void Initialize_ButtonMatrix();
u32 ScanButtons();

/* Power functions */
void Initialize_Clock(void);
void Initialize_PowerSwitch(void);
int CheckPowerSwitch();
void PowerDown();

/* Sticks */
void Initialize_Channels();
u16 ReadThrottle();
u16 ReadRudder();
u16 ReadElevator();
u16 ReadAileron();

/* SPI Flash */
void Initialize_SPIFlash();
u32 ReadFlashID();
void EraseSector(u32 sectorAddress);
void WritePage(u32 writeAddress, u32 length, u8 * buffer);

/* SPI CYRF6936 */
void Initialize_SPICYRF();
void GetMfgData(u8 data[]);

void ConfigRxTx(u32 TxRx);
void ConfigRFChannel(u8 ch);
void ConfigCRCSeed(u8 crc);
void StartReceive();

/* UART & Debug */
void Initialize_UART();

#ifndef SKIP_PRINTF_DEFS
int printf(const char *format, ...);
int sprintf(char *out, const char *format, ...);
#endif
#endif
