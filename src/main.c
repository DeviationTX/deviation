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

#include "target.h"
#include "misc.h"
#include "gui/gui.h"

int main() {
    PWR_Init();
    Initialize_ButtonMatrix();
    Delay(0x2710);

    LCD_Init();
    Initialize_Channels();

    SPIFlash_Init();
    CYRF_Initialize();
    UART_Initialize();
    SPITouch_Init();
    USB_Initialize();
    SPI_FlashBlockWriteEnable(1); //Enable writing to all banks of SPIFlash
    SignOn();
    LCD_Clear(0x0000);

#ifdef BL_DUMP
    LCD_PrintStringXY(40, 10, "Dumping");

    printf("Erase...\n\r");

    SPIFlash_EraseSector(0x2000);
    SPIFlash_EraseSector(0x3000);
    SPIFlash_EraseSector(0x4000);
    SPIFlash_EraseSector(0x5000);

    printf("Pgm 2\n\r");
    SPIFlash_WriteBytes(0x2000, 0x1000, (u8*)0x08000000);
    printf("Pgm 3\n\r");
    SPIFlash_WriteBytes(0x3000, 0x1000, (u8*)0x08001000);
    printf("Pgm 4\n\r");
    SPIFlash_WriteBytes(0x4000, 0x1000, (u8*)0x08002000);
    printf("Pgm 5\n\r");
    SPIFlash_WriteBytes(0x5000, 0x1000, (u8*)0x08003000);
    printf("Done\n\r");

    LCD_Clear(0x0000);
    LCD_PrintStringXY(40, 10, "Done");

    while(1)
    {
        if(PWR_CheckPowerSwitch())
        PWR_Shutdown();
    }
#endif 

#ifdef STATUS_SCREEN
    /* Some needed variables */
    u32 last_buttons = 0;

    char strBootLoader[80],strSPIFlash[80],strMfg[80];
    char buttonstr[80];
    char buttonstring[33];
    char voltagestr[8];
    char te[40];
    char ra[40];
    char t1[40];
    char t2[40];
    char buttonmessage[30];
    const char *button1 = " Button 1 ";
    const char *button2 = " Button 2 ";
    const char *button3 = " Button 3 ";
    const char *statusBar = "bar.bmp";
    const char *batteryImg = "bat.bmp";
    int frmStatusBar;
    int frmBattery;
    int lblButtonMessage;
    int lblButtons;
    int lblVoltage;
    int lblTE;
    int lblRA;
    int lblT1;
    int lblT2;
    int lblSPIFlash;
    int lblBootLoader;
    int lblMfg;

    sprintf(buttonstr,"buttons");
    sprintf(buttonmessage," ");
    sprintf(t1," ");
    sprintf(t2," ");

    /* GUI Callbacks */
    void PushMeButton1(int objID) {
        GUI_RemoveObj(objID);
        sprintf(buttonmessage,"%s","Button 1 Pushed");
    }
    void PushMeButton2(int objID) {
        GUI_RemoveObj(objID);
        sprintf(buttonmessage,"%s","Button 2 Pushed");
    }
    void PushMeButton3(int objID) {
        GUI_RemoveObj(objID);
        sprintf(buttonmessage,"%s","Button 3 Pushed");
    }
    void openDialogPush(int objID, struct guiDialogReturn gDR) {
        GUI_RemoveObj(objID);
    }

    {
        u8 * pBLString = BOOTLOADER_Read(BL_ID);
        (u8*)0x08001000;
        u8 mfgdata[6];
        sprintf(strBootLoader, "BootLoader   : %s\n",pBLString);
        sprintf(strSPIFlash, "SPI Flash    : %X\n",(unsigned int)SPIFlash_ReadID());
        CYRF_GetMfgData(mfgdata);
        sprintf(strMfg, "CYRF Mfg Data: %02X%02X%02X%02X%02X%02X\n",
                mfgdata[0],
                mfgdata[1],
                mfgdata[2],
                mfgdata[3],
                mfgdata[4],
                mfgdata[5]);
    }

    /* Create some GUI elements */

    frmStatusBar = GUI_CreateFrame(0,0,320,24,statusBar);
    frmBattery = GUI_CreateFrame(270,1,48,22,batteryImg);
    lblSPIFlash = GUI_CreateLabel(10,50,strSPIFlash,0x0000);
    lblBootLoader = GUI_CreateLabel(10,40,strBootLoader,0x0000);
    lblMfg = GUI_CreateLabel(10,30,strMfg,0x0000);
    lblButtons = GUI_CreateLabel(10,60,buttonstr,0x0000);
    lblVoltage = GUI_CreateLabel(262,8,voltagestr,0x0f00);
    lblTE = GUI_CreateLabel(10,90,te,0xffff);
    lblRA = GUI_CreateLabel(10,100,ra,0xffff);
    lblT1 = GUI_CreateLabel(10,110,t1,0xffff);
    lblT2 = GUI_CreateLabel(10,120,t2,0xffff);
    lblButtonMessage = GUI_CreateLabel(100,130,buttonmessage,0xffff);
    int testButton1 = GUI_CreateButton(10,200,89,23,button1,0x0000,PushMeButton1);
    int testButton2 = GUI_CreateButton(110,200,89,23,button2,0x0000,PushMeButton2);
    int testButton3 = GUI_CreateButton(210,200,89,23,button3,0x0000,PushMeButton3);
    int openDialog = GUI_CreateDialog(70,50,180,130,"Deviation","     Welcome to\n     Deviation",0xffff,0x0000,openDialogPush,dtOk);

    /* little bit of code to stop variable warnings */
    if (frmStatusBar && frmBattery && lblButtonMessage && lblButtons && lblVoltage &&
        lblTE && lblRA && lblT1 && lblT2 && lblSPIFlash && lblBootLoader && lblMfg &&
        testButton1 && testButton2 && testButton3 && openDialog)
    {/*Just here to avoid warnings*/}

    int ReDraw=1;
    while(1) {
        int i;
        if(PWR_CheckPowerSwitch())
        PWR_Shutdown();
        u32 buttons = ScanButtons();

        /* GUI Handling
         * We beed to handle screen redraws here
         * */
        if(buttons != last_buttons) {
            last_buttons = buttons;
            if((buttons & 0x1) == 0)
            LCD_CalibrateTouch();
            for(i = 0; i < 32; i++)
            buttonstring[i] = (buttons & (1 << i)) ? '0' : '1';
        }

        char s[80];
        sprintf(s,"Buttons:\n%s",buttonstring);
        if (strcmp(s,buttonstr) != 0) {
            sprintf(buttonstr,"%s",s);
            ReDraw = 1;
        }
        u16 voltage = PWR_ReadVoltage();
        sprintf(s, "%2d.%03d\n", voltage >> 12, voltage & 0x0fff);
        if (strcmp(s,voltagestr) != 0) {
            sprintf(voltagestr,"%s",s);
            ReDraw = 1;
        }
        u16 throttle = ReadThrottle();
        u16 rudder = ReadRudder();
        u16 aileron = ReadAileron();
        u16 elevator = ReadElevator();
        sprintf(s, "Throttle: %04X Elevator: %04X\n", throttle, elevator);
        if (strcmp(s,te) != 0) {
            sprintf(te,"%s",s);
            ReDraw = 1;
        }
        sprintf(s, "Rudder  : %04X Aileron : %04X\n", rudder, aileron);
        if (strcmp(s,ra) != 0) {
            sprintf(ra,"%s",s);
            ReDraw = 1;
        }
        if(SPITouch_IRQ()) {
            struct touch t = SPITouch_GetCoords();

            sprintf(s, "x : %4d y : %4d\n", t.x, t.y);
            if (strcmp(s,t1) != 0) {
                sprintf(t1,"%s",s);
                ReDraw = 1;
            }
            sprintf(s, "z1: %4d z2: %4d\n", t.z1, t.z2);
            if (strcmp(s,t2) != 0) {
                sprintf(t2,"%s",s);
                ReDraw = 1;
            }
            GUI_CheckTouch(t);
        }
        if (ReDraw == 1) {
            /* Redraw everything */
            GUI_DrawScreen();
            ReDraw = 0;
        }
    }
#endif    
#ifdef SCANNER
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

        printf("%02X : %d\n\r",channel,channelnoise[channel]);

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

#endif
    return 0;
}

