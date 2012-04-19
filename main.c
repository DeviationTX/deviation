//###############################
//Multi-Inno MIO283QT-2 (HX8347D)
//###############################
#include <libopencm3/stm32/f1/flash.h>

#include "tx.h"

void Delay(u32 count)
{
    while(count) {
        int i = 72000;
        while(i)
            i--;
        count--;
    }
}

void Hang()
{
    while(1)
        ;
}

int main()
{

    Initialize_PowerSwitch();
    Initialize_ButtonMatrix();
    Delay(0x2710);

    flash_unlock();

    Initialize_Clock();

    Initialize_LCD();
    Initialize_Backlight();
    Initialize_Channels();

    Initialize_SPIFlash();
    Initialize_UART();
    SignOn();

    lcd_clear(0x0000);
    lcd_writeCharacter('H', 40, 10);    
    lcd_writeCharacter('E', 50, 10);    
    lcd_writeCharacter('L', 60, 10);    
    lcd_writeCharacter('L', 70, 10);    
    lcd_writeCharacter('O', 80, 10);    
    lcd_writeCharacter('W', 40, 30);    
    lcd_writeCharacter('O', 50, 30);    
    lcd_writeCharacter('R', 60, 30);    
    lcd_writeCharacter('L', 70, 30);    
    lcd_writeCharacter('D', 80, 30);    

    while(1) {
        int i;
        if(CheckPowerSwitch())
            PowerDown();
        u32 buttons = ScanButtons();
        for(i = 0; i < 32; i++)
            lcd_writeCharacter((buttons & (1 << i)) ? '0' : '1', i << 3, 60);
        u16 throttle = ReadThrottle();
        for(i = 11; i >= 0; i--)
            lcd_writeCharacter((throttle & (1 << i)) ? '0' : '1', i << 3, 80);
    }
}

