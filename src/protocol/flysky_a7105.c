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

#include "target.h"
#include "interface.h"
#include "mixer.h"

#ifdef PROTO_HAS_A7105

static const u8 A7105_regs[] = {
     -1,  0x42, 0x00, 0x14, 0x00,  -1 ,  -1 , 0x00, 0x00, 0x00, 0x00, 0x01, 0x21, 0x05, 0x00, 0x50,
    0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x00, 0x62, 0x80, 0x80, 0x00, 0x0a, 0x32, 0xc3, 0x0f,
    0x13, 0xc3, 0x00,  -1,  0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,
    0x01, 0x0f,  -1,
};
static const u8 channels[] = {
    0x0a, 0x5a, 0x50, 0xa0, 0x14, 0x64, 0x46, 0x96, 0x1e, 0x6e, 0x3c, 0x8c, 0x28, 0x78, 0x32, 0x82
};
static const u8 *chanptr;
static u8 packet[22];
static u16 counter;

static void flysky_init()
{
    int i;
    u8 if_calibration1;
    u8 vco_calibration0;
    u8 vco_calibration1;

    A7105_WriteID(0x5475c52a);
    for (i = 0; i < 0x33; i++)
        if((s8)A7105_regs[i] != -1)
            A7105_WriteReg(i, A7105_regs[i]);

    A7105_Strobe(A7105_STANDBY);

    //IF Filter Bank Calibration
    A7105_WriteReg(0x02, 1);
    A7105_ReadReg(0x02);
    while(A7105_ReadReg(0x02))
        ;
    if_calibration1 = A7105_ReadReg(0x22);
    if(if_calibration1 & A7105_MASK_FBCF) {
        //Calibration failed...what do we do?
    }

    //VCO Current Calibration
    A7105_WriteReg(0x24, 0x13); //Recomended calibration from A7105 Datasheet

    //VCO Bank Calibration
    A7105_WriteReg(0x26, 0x3b); //Recomended limits from A7105 Datasheet

    //VCO Bank Calibrate channel 0?
    //Set Channel
    A7105_WriteReg(0x0f, 0); //Should we choose a different channel?
    //VCO Calibration
    A7105_WriteReg(0x02, 2);
    while(A7105_ReadReg(0x02))
        ;
    vco_calibration0 = A7105_ReadReg(0x25);
    if (vco_calibration0 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
    }

    //Calibrate channel 0xa0?
    //Set Channel
    A7105_WriteReg(0x0f, 0xa0); //Should we choose a different channel?
    //VCO Calibration
    A7105_WriteReg(0x02, 2);
    while(A7105_ReadReg(0x02))
        ;
    vco_calibration1 = A7105_ReadReg(0x25);
    if (vco_calibration1 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
    }

    //Reset VCO Band calibration
    A7105_WriteReg(0x25, 0x08);

    A7105_Strobe(A7105_STANDBY);
}

static void flysky_build_packet(u8 init)
{
    int i;
    //-100% =~ 0x03e8
    //+100% =~ 0x07ca
    //Calculate:
    //Center = 0x5d9
    //1 %    = 5
    packet[0] = 0x05;
    packet[1] = init ? 0xaa : 0x55;
    packet[2] = 0xF3;
    packet[3] = 0x01;
    packet[4] = 0x00;  //0x2000 is the 'unique id'
    packet[5] = 0x20;
    for (i = 0; i < 8; i++) {
        if (i > NUM_CHANNELS) {
            packet[6 + i*2] = 0;
            packet[7 + i*2] = 0;
            continue;
        }
        s32 value = (s32)Channels[i] * 0x1f1 / CHAN_MAX_VALUE + 0x5d9;
        if (value < 0)
            value = 0;
        packet[6 + i] = value & 0xff;
        packet[7 + i] = (value >> 8) & 0xff;
    }
}
static u16 flysky_cb()
{
    if (counter) {
        flysky_build_packet(1);
        A7105_WriteData(packet, 22, 1);
        counter--;
    } else {
        flysky_build_packet(0);
        A7105_WriteData(packet, 22, *chanptr);
        if (chanptr != channels + sizeof(channels) - 1)
           chanptr++;
        else
           chanptr = channels;
    }
    return 1460;
}

void FLYSKY_Initialize() {
    CLOCK_StopTimer();
    A7105_Reset();
    flysky_init();
    counter = 2500;
    chanptr = channels;
    CLOCK_StartTimer(2400, flysky_cb);
}

#endif
