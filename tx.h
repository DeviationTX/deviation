#ifndef _TX_H_
#define _TX_H_

#include <libopencm3/cm3/common.h>

/* General functions */
void Delay(u32 count);
void Hang();

/* Display Functions */
void Initialize_Backlight();
void Initialize_LCD();
void lcd_writeCharacter(unsigned char character, unsigned int x, unsigned int y);
void lcd_clear(unsigned int color);

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

/* USART */
void Initialize_UART();

#endif
