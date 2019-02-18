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

#ifndef _ILI9341_H_
#define _ILI9341_H_

#define ILI9341_00_DEVICE_CODE_READ_REG   0x00
#define ILI9341_01_SOFT_RESET_REG         0x01
#define ILI9341_04_IDENTINFO_R_REG        0x04
#define ILI9341_09_STATUS_R_REG           0x09
#define ILI9341_0A_POWERMODE_R_REG        0x0A
#define ILI9341_0B_MADCTL_R_REG           0x0B
#define ILI9341_0C_PIXFORMAT_R_REG        0x0C
#define ILI9341_0D_IMGFORMAT_R_REG        0x0D
#define ILI9341_0E_SIGMODE_R_REG          0x0E
#define ILI9341_0F_SD_RESULT_R_REG        0x0F
#define ILI9341_10_SLEEP_ENTER_REG        0x10
#define ILI9341_11_SLEEP_OUT_REG          0x11
#define ILI9341_12_PARTIALMODE_REG        0x12
#define ILI9341_13_NORDISPMODE_REG        0x13
#define ILI9341_20_INVERSIONOFF_REG       0x20
#define ILI9341_21_INVERSIONON_REG        0x21
#define ILI9341_26_GAMMASET_REG           0x26
#define ILI9341_28_DISPLAYOFF_REG         0x28
#define ILI9341_29_DISPLAYON_REG          0x29
#define ILI9341_2A_COLADDRSET_REG         0x2A
#define ILI9341_2B_PAGEADDRSET_REG        0x2B
#define ILI9341_2C_MEMORYWRITE_REG        0x2C
#define ILI9341_2D_COLORSET_REG           0x2D
#define ILI9341_2E_MEMORYREAD_REG         0x2E
#define ILI9341_30_PARTIALAREA_REG        0x30
#define ILI9341_33_VERTSCROLL_REG         0x33
#define ILI9341_34_TEAREFFECTLINEOFF_REG  0x34
#define ILI9341_35_TEAREFFECTLINEON_REG   0x35
#define ILI9341_36_MEMACCESS_REG          0x36
#define ILI9341_37_VERSCRSRART_REG        0x37
#define ILI9341_38_IDLEMODEOFF_REG        0x38
#define ILI9341_39_IDLEMODEON_REG         0x39
#define ILI9341_3A_PIXFORMATSET_REG       0x3A
#define ILI9341_3C_WRITEMEMCONTINUE_REG   0x3C
#define ILI9341_3E_READMEMCONTINUE_REG    0x3E
#define ILI9341_44_SETTEATSCAN_REG        0x44
#define ILI9341_45_GETSCANLINE_REG        0x45
#define ILI9341_51_WRITEBRIGHT_REG        0x51
#define ILI9341_52_READBRIGHT_REG         0x52
#define ILI9341_53_WRITECTRL_REG          0x53
#define ILI9341_54_READCTRL_REG           0x54
#define ILI9341_55_WRITECABC_REG          0x55
#define ILI9341_56_READCABC_REG           0x56
#define ILI9341_5E_WRITECABCMB_REG        0x5E
#define ILI9341_5F_READCABCMB_REG         0x5F
#define ILI9341_B0_RGB_ISCTL_REG          0xB0
#define ILI9341_B1_FRAMECTL_NOR_REG       0xB1
#define ILI9341_B2_FRAMECTL_IDLE_REG      0xB2
#define ILI9341_B3_FRAMECTL_PARTIAL_REG   0xB3
#define ILI9341_B4_INVERCTL_REG           0xB4
#define ILI9341_B5_BLANKPORCTL_REG        0xB5
#define ILI9341_B6_FUNCTONCTL_REG         0xB6
#define ILI9341_B7_ENTRYMODE_REG          0xB7
#define ILI9341_B8_BLIGHTCTL1_REG         0xB8
#define ILI9341_B9_BLIGHTCTL2_REG         0xB9
#define ILI9341_BA_BLIGHTCTL3_REG         0xBA
#define ILI9341_BB_BLIGHTCTL4_REG         0xBB
#define ILI9341_BC_BLIGHTCTL5_REG         0xBC
#define ILI9341_BE_BLIGHTCTL7_REG         0xBE
#define ILI9341_BF_BLIGHTCTL8_REG         0xBF
#define ILI9341_C0_POWERCTL1_REG          0xC0
#define ILI9341_C1_POWERCTL2_REG          0xC1
#define ILI9341_C5_VCOMCTL1_REG           0xC5
#define ILI9341_C7_VCOMCTL2_REG           0xC7
#define ILI9341_CB_POWERCTLA_REG          0xCB
#define ILI9341_CF_POWERCTLB_REG          0xCF
#define ILI9341_D0_NVMEMWRITE_REG         0xD0
#define ILI9341_D1_NVMEMPROTECTKEY_REG    0xD1
#define ILI9341_D2_NVMEMSTATUS_REG        0xD2
#define ILI9341_D3_READID4_REG            0xD3
#define ILI9341_DA_READID1_REG            0xDA
#define ILI9341_DB_READID2_REG            0xDB
#define ILI9341_DC_READID3_REG            0xDC
#define ILI9341_E0_POSGAMMACORRECTION_REG 0xE0
#define ILI9341_E1_NEGGAMMACORRECTION_REG 0xE1
#define ILI9341_E2_DIGGAMCTL1_REG         0xE2
#define ILI9341_E3_DIGGAMCTL2_REG         0xE3
#define ILI9341_E8_DIVTIMCTL_A_REG        0xE8
#define ILI9341_EA_DIVTIMCTL_B_REG        0xEA
#define ILI9341_ED_POWONSEQCTL_REG        0xED
#define ILI9341_F2_ENABLE_3G_REG          0xF2
#define ILI9341_F6_INTERFCTL_REG          0xF6
#define ILI9341_F7_PUMPRATIOCTL_REG       0xF7

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
#endif
