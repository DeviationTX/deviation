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

#ifdef MODULAR
  //Allows the linker to properly relocate
  #define DEVO_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "config/tx.h"
#include "protocol/interface.h"
#include "protospi.h"

#ifdef PROTO_HAS_A7105

static void CS_HI() {
    PROTO_CS_HI(A7105);
}

static void CS_LO() {
    PROTO_CS_LO(A7105);
}

void A7105_WriteReg(u8 address, u8 data)
{
    CS_LO();
    PROTOSPI_xfer(address);
    PROTOSPI_xfer(data);
    CS_HI();
}

u8 A7105_ReadReg(u8 address)
{
    u8 data;
    CS_LO();
    PROTOSPI_xfer(0x40 | address);
    /* Wait for tx completion before spi shutdown */
    data = PROTOSPI_read3wire();
    CS_HI();
    return data;
}

void A7105_WriteData(u8 *dpbuffer, u8 len, u8 channel)
{
    int i;
    CS_LO();
    PROTOSPI_xfer(A7105_RST_WRPTR);
    PROTOSPI_xfer(0x05);
    for (i = 0; i < len; i++)
        PROTOSPI_xfer(dpbuffer[i]);
    CS_HI();

    A7105_WriteReg(0x0F, channel);

    CS_LO();
    PROTOSPI_xfer(A7105_TX);
    CS_HI();
}
void A7105_ReadData(u8 *dpbuffer, u8 len)
{
    A7105_Strobe(A7105_RST_RDPTR);
    for(int i = 0; i < len; i++)
        dpbuffer[i] = A7105_ReadReg(0x05);
    return;
}

/*
 * 1 - Tx else Rx
 */
void A7105_SetTxRxMode(enum TXRX_State mode)
{
    if(mode == TX_EN) {
        A7105_WriteReg(A7105_0B_GPIO1_PIN1, 0x33);
        A7105_WriteReg(A7105_0C_GPIO2_PIN_II, 0x31);
    } else if (mode == RX_EN) {
        A7105_WriteReg(A7105_0B_GPIO1_PIN1, 0x31);
        A7105_WriteReg(A7105_0C_GPIO2_PIN_II, 0x33);
    } else {
        //The A7105 seems to some with a cross-wired power-amp (A7700)
        //On the XL7105-D03, TX_EN -> RXSW and RX_EN -> TXSW
        //This means that sleep mode is wired as RX_EN = 1 and TX_EN = 1
        //If there are other amps in use, we'll need to fix this
        A7105_WriteReg(A7105_0B_GPIO1_PIN1, 0x33);
        A7105_WriteReg(A7105_0C_GPIO2_PIN_II, 0x33);
    }
}

int A7105_Reset()
{
    A7105_WriteReg(0x00, 0x00);
    usleep(1000);
    //Set both GPIO as output and low
    A7105_SetTxRxMode(TXRX_OFF);
    int result = A7105_ReadReg(0x10) == 0x9E;
    A7105_Strobe(A7105_STANDBY);
    return result;
    
}
void A7105_WriteID(u32 id)
{
    CS_LO();
    PROTOSPI_xfer(0x06);
    PROTOSPI_xfer((id >> 24) & 0xFF);
    PROTOSPI_xfer((id >> 16) & 0xFF);
    PROTOSPI_xfer((id >> 8) & 0xFF);
    PROTOSPI_xfer((id >> 0) & 0xFF);
    CS_HI();
}

void A7105_SetPower(int power)
{
    /*
    Power amp is ~+16dBm so:
    TXPOWER_100uW  = -23dBm == PAC=0 TBG=0
    TXPOWER_300uW  = -20dBm == PAC=0 TBG=1
    TXPOWER_1mW    = -16dBm == PAC=0 TBG=2
    TXPOWER_3mW    = -11dBm == PAC=0 TBG=4
    TXPOWER_10mW   = -6dBm  == PAC=1 TBG=5
    TXPOWER_30mW   = 0dBm   == PAC=2 TBG=7
    TXPOWER_100mW  = 1dBm   == PAC=3 TBG=7
    TXPOWER_150mW  = 1dBm   == PAC=3 TBG=7
    */
    u8 pac, tbg;
    switch(power) {
        case 0: pac = 0; tbg = 0; break;
        case 1: pac = 0; tbg = 1; break;
        case 2: pac = 0; tbg = 2; break;
        case 3: pac = 0; tbg = 4; break;
        case 4: pac = 1; tbg = 5; break;
        case 5: pac = 2; tbg = 7; break;
        case 6: pac = 3; tbg = 7; break;
        case 7: pac = 3; tbg = 7; break;
        default: pac = 0; tbg = 0; break;
    };
    A7105_WriteReg(0x28, (pac << 3) | tbg);
}

void A7105_Strobe(enum A7105_State state)
{
    CS_LO();
    PROTOSPI_xfer(state);
    CS_HI();
}

// fine tune A7105 LO base frequency
// this is required for some A7105 modules / Rx
// with inaccurate crystal oscillator
// arg: offset in +/-kHz
void A7105_AdjustLOBaseFreq(s16 offset)
{
    // LO base frequency = 32e6*(bip+(bfp/(2^16)))
    u8 bip;  // LO base frequency integer part
    u16 bfp; // LO base frequency fractional part
    if(offset < 0) {
        bip = 0x4a; // 2368 MHz
        bfp = 0xffff + (offset * 2.048f) + 1;
    }
    else {
        bip = 0x4b; // 2400 MHz (default)
        bfp = offset * 2.048f;
    }
    if(offset == 0)
        bfp = 0x0002; // as per datasheet, not sure why recommanded, but that's a +1kHz drift only ...
    A7105_WriteReg( A7105_11_PLL_III, bip);
    A7105_WriteReg( A7105_12_PLL_IV, (bfp >> 8) & 0xff);
    A7105_WriteReg( A7105_13_PLL_V, bfp & 0xff);
}

//#pragma long_calls_off
#endif
