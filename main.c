/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

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

