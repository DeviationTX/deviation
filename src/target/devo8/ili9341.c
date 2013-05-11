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

    This file is partially based upon the CooCox ILI9341S driver
    http://www.coocox.org/driver_repo/305488dd-734b-4cce-a8a4-39dcfef8cc66/html/group___i_l_i9341_s.html
*/

#include "lcd.h"
#include "common.h"
#include "ili9341.h"

//static unsigned int _x1;
//static unsigned int _y1;
void ili9341_set_pos(unsigned int x0, unsigned int y0)
{
    LCD_REG  = ILI9341_2A_COLADDRSET_REG;
    LCD_DATA = (u8)(x0 >> 8);
    LCD_DATA = (u8)(x0);
    //LCD_DATA = (u8)(_x1 >> 8);
    //LCD_DATA = (u8)(_x1);
    //set y0
    LCD_REG  = ILI9341_2B_PAGEADDRSET_REG;
    LCD_DATA = (u8)(y0 >> 8);
    LCD_DATA = (u8)(y0);
    //LCD_DATA = (u8)(_y1 >> 8);
    //LCD_DATA = (u8)(_y1);
    //Set draw mode
    LCD_REG = ILI9341_2C_MEMORYWRITE_REG;
}

void ili9341_draw_start(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
    LCD_REG  = ILI9341_36_MEMACCESS_REG;
    if (screen_flip) {
        LCD_DATA = 0x28;     //MY=0 MX=0 MV=1 ML=0 BGR=1
    } else {
        LCD_DATA = 0x68;     //MY=0 MX=1 MV=1 ML=0 BGR=1
    }    
    //_x1 = x1;
    //_y1 = y1;
    //set x0
    LCD_REG  = ILI9341_2A_COLADDRSET_REG;
    LCD_DATA = (u8)(x0 >> 8);
    LCD_DATA = (u8)(x0);
    LCD_DATA = (u8)(x1 >> 8);
    LCD_DATA = (u8)(x1);
    //set y0
    LCD_REG  = ILI9341_2B_PAGEADDRSET_REG;
    LCD_DATA = (u8)(y0 >> 8);
    LCD_DATA = (u8)(y0);
    LCD_DATA = (u8)(y1 >> 8);
    LCD_DATA = (u8)(y1);
    //Set draw mode
    LCD_REG = ILI9341_2C_MEMORYWRITE_REG;
}

void ili9341_sleep()
{
    LCD_REG = ILI9341_10_SLEEP_ENTER_REG;
}

static const struct lcdtype ili9341_type = {
    ili9341_set_pos,
    ili9341_draw_start,
    ili9341_sleep,
};

void ili9341_init()
{
    //Power Control B
    LCD_REG  = ILI9341_CF_POWERCTLB_REG;
    LCD_DATA = 0x00;
    LCD_DATA = 0x81;
    LCD_DATA = 0x30;
    //Power On Sequence Control
    LCD_REG  = ILI9341_ED_POWONSEQCTL_REG;
    LCD_DATA = 0x64;
    LCD_DATA = 0x03;
    LCD_DATA = 0x12;
    LCD_DATA = 0x81;
    //Driver Timing Control A
    LCD_REG  = ILI9341_E8_DIVTIMCTL_A_REG;
    LCD_DATA = 0x85;
    LCD_DATA = 0x10;
    LCD_DATA = 0x78;
    //Power Control A
    LCD_REG  = ILI9341_CB_POWERCTLA_REG;
    LCD_DATA = 0x39;
    LCD_DATA = 0x2c;
    LCD_DATA = 0x00;
    LCD_DATA = 0x34;
    LCD_DATA = 0x02;
    //Pump Ratio Control
    LCD_REG  = ILI9341_F7_PUMPRATIOCTL_REG;
    LCD_DATA = 0x20;
    //Driver Timing Control B
    LCD_REG  = ILI9341_EA_DIVTIMCTL_B_REG;
    LCD_DATA = 0x00;
    LCD_DATA = 0x00;
    //Frame Rate Control
    LCD_REG  = ILI9341_B1_FRAMECTL_NOR_REG;
    LCD_DATA = 0x00;
    LCD_DATA = 0x1b;
    //Display Function Control
    LCD_REG  = ILI9341_B6_FUNCTONCTL_REG;
    LCD_DATA = 0x0a;
    LCD_DATA = 0xA2;
    //LCD_DATA = xx
    //LCD_DATA = xx
    //Power Control 1
    LCD_REG  = ILI9341_C0_POWERCTL1_REG;
    LCD_DATA = 0x24;
    //Power Control 2
    LCD_REG  = ILI9341_C1_POWERCTL2_REG;
    LCD_DATA = 0x11;
    //VCOM Control 1
    LCD_REG  = ILI9341_C5_VCOMCTL1_REG;
    LCD_DATA = 0x24;
    LCD_DATA = 0x34;
    //VCOM Control 2
    LCD_REG  = ILI9341_C7_VCOMCTL2_REG;
    LCD_DATA = 0xb5;
    //Memory Access Control
    LCD_REG  = ILI9341_36_MEMACCESS_REG;
    LCD_DATA = 0x68;
    //COLMOD Pixel Format Scan
    LCD_REG  = ILI9341_3A_PIXFORMATSET_REG;
    LCD_DATA = 0x05;
    //Enable 3 Gamma
    LCD_REG  = ILI9341_F2_ENABLE_3G_REG;
    LCD_DATA = 0x00;
    //Gamma Set
    LCD_REG  = ILI9341_26_GAMMASET_REG;
    LCD_DATA = 0x01;
    //Positive Gamma Correction
    LCD_REG  = ILI9341_E0_POSGAMMACORRECTION_REG;
    LCD_DATA = 0x0f;
    LCD_DATA = 0x26;
    LCD_DATA = 0x24;
    LCD_DATA = 0x0b;
    LCD_DATA = 0x0e;
    LCD_DATA = 0x09;
    LCD_DATA = 0x54;
    LCD_DATA = 0xa8;
    LCD_DATA = 0x46;
    LCD_DATA = 0x0c;
    LCD_DATA = 0x17;
    LCD_DATA = 0x09;
    LCD_DATA = 0x0f;
    LCD_DATA = 0x07;
    LCD_DATA = 0x00;
    //Negative Gamma Correction
    LCD_REG  = ILI9341_E1_NEGGAMMACORRECTION_REG;
    LCD_DATA = 0x00;
    LCD_DATA = 0x19;
    LCD_DATA = 0x1b;
    LCD_DATA = 0x04;
    LCD_DATA = 0x10;
    LCD_DATA = 0x07;
    LCD_DATA = 0x2a;
    LCD_DATA = 0x47;
    LCD_DATA = 0x39;
    LCD_DATA = 0x03;
    LCD_DATA = 0x06;
    LCD_DATA = 0x06;
    LCD_DATA = 0x30;
    LCD_DATA = 0x38;
    LCD_DATA = 0x0f;
    //Sleep OUT
    LCD_REG  = ILI9341_11_SLEEP_OUT_REG;
    Delay(120);
    //Display ON
    LCD_REG = ILI9341_29_DISPLAYON_REG;
    Delay(5);
    disp_type = &ili9341_type;
}
