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

#include <stdlib.h>
static void error(char *str);
static int ReadHex(char *file, u8 *data, int size);

struct Transmitter Transmitter;

struct {
    u32 id;
    const char *str;
    int page_size;
} *avr, AVR[] = {
    {0x0B911E53, "tiny24a", 32},
    {0x07921E53, "tiny44a", 64},
    {0x0C931E53, "tiny84a", 64},
    {0, "", 0},
};

int main() {
    PWR_Init();
    CLOCK_Init();
    UART_Initialize();
    Initialize_ButtonMatrix();
    SPIFlash_Init(); //This must come before LCD_Init() for 7e

    if(PWR_CheckPowerSwitch()) PWR_Shutdown();
    LCD_Init();
    BACKLIGHT_Init();
    BACKLIGHT_Brightness(1);
    LCD_Clear(0x0000);

    u32 buttons = ScanButtons();
    if(CHAN_ButtonIsPressed(buttons, BUT_ENTER) || !FS_Mount(NULL, NULL)) {
        SPI_FlashBlockWriteEnable(1); //Enable writing to all banks of SPIFlash
        LCD_DrawUSBLogo(LCD_WIDTH, LCD_HEIGHT);
        USB_Connect();
        LCD_Clear(0x0000);
        FS_Mount(NULL, NULL);
    }
    memset(&Transmitter, 0, sizeof(Transmitter));
    SPI_ProtoInit();
    CONFIG_LoadTx();
    CONFIG_ReadDisplay();
    BACKLIGHT_Brightness(Transmitter.brightness);
    LCD_Contrast(Transmitter.contrast);
    LCD_SetFont(DEFAULT_FONT.font);
    LCD_SetFontColor(0xffff);
    if (! Transmitter.module_enable[PROGSWITCH].port) {
        error("ERR: No switch cfg");
    }
    u8 data[2048]; //must be a multiple of 64
    memset(data, 0, sizeof(data));
    int size = ReadHex("avr.hex", data, sizeof(data));
    if(! size) {
        error("ERR: Bad avr.hex");
    }
    if (size > (int)sizeof(data)) {
        error("ERR: Hex too Large");
    }
    //Initialize and detect AVR
    LCD_PrintStringXY(0,0, "1. Init");
    usleep(500000);
    u32 type = AVR_StartProgram();
    if (! type) {
        error("ERR: No switch found");
    }
    int i = 0;
    avr = NULL;
    //Determine AVR type
    while(AVR[i].id != 0) {
        if(type == AVR[i].id) {
            avr = &AVR[i];
            break;
        }
        i++;
    }
    if (! avr) {
        sprintf(tempstring, "ERR: Unknown AVR %08x", type);
        error(tempstring);
    }
    char avr_str[100];
    sprintf(avr_str, "Found AVR: %08x\nhex: %dbytes", type, size);
    //Erase AVR
    sprintf(tempstring, "%s\n2. Erasing", avr_str);
    LCD_Clear(0x0000);
    LCD_PrintStringXY(0,0, tempstring);
    if(! AVR_Erase()) {
        sprintf(tempstring, "%s\nERR: Erase failed", avr_str);
        error(tempstring);
    }
    sprintf(tempstring, "%s\n2. Programming", avr_str);
    LCD_Clear(0x0000);
    LCD_PrintStringXY(0,0, tempstring);
    i = 0;
    while(i < size) {
        if(! AVR_Program(i / 2, &data[i], avr->page_size)) {
            sprintf(tempstring, "%s\nERR:Failed at page 0x%04x", avr_str, i);
            error(tempstring);
        }
        i += avr->page_size;
    }
#if 0
    //Untested
    sprintf(tempstring, "%s\n3. Setting Fuses", avr_str);
    LCD_Clear(0x0000);
    LCD_PrintStringXY(0,0, tempstring);
    if(!AVR_SetFuses()) {
        sprintf(tempstring, "%s\nERR: Couldn't set fuses", avr_str);
        error(tempstring);
    }
#endif
    sprintf(tempstring, "%s\n3. Done", avr_str);
    LCD_Clear(0x0000);
    LCD_PrintStringXY(0,0, tempstring);
    while(1)
        if(PWR_CheckPowerSwitch()) PWR_Shutdown();
        
}

void error(char *str)
{
    LCD_Clear(0x0000);
    LCD_PrintStringXY(0,0, str);
    while(1)
        if(PWR_CheckPowerSwitch()) PWR_Shutdown();
}

static int hex2int(char *data, unsigned len)
{
    char tmp[9];
    strncpy(tmp, data, len);
    tmp[len] = '\0';
    return strtol(tmp, NULL, 16);
}

int ReadHex(char *filename, u8 *data, int size)
{
    int data_size = 0;
    char line[64];
    FILE *fh = fopen(filename, "r");
    if(! fh)
        return 0;
    while (fgets(line, sizeof(line), fh) != NULL) {
        if(line[0] != ':')
            continue;
        if(strlen(line) < 11) {
            fclose(fh);
            return 0;
        }
        int count = hex2int(line+1, 2);
        int addr  = hex2int(line+3, 4);
        int type  = hex2int(line+7, 2);
        if (type == 0) {
            if (addr + count > data_size)
                data_size = addr + count;
            if (addr + count > size)
                continue; //do not overflow the array
            for(int i = 0; i < count; i++) {
                data[addr+i] = hex2int(line+9+(2*i), 2);
            }
        } else if(type == 1) {
            break;
        }
    }
    fclose(fh);
    return data_size;
}
