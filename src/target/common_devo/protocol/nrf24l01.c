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

#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "config/tx.h"
#include "protocol/interface.h"

#ifdef PROTO_HAS_NRF24L01

/* Instruction Mnemonics */
#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define ACTIVATE      0x50
#define R_RX_PL_WID   0x60
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define W_ACK_PAYLOAD 0xA8
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define NOP           0xFF

//GPIOA.14
#define CS_HI() gpio_set(Transmitter.module_enable[NRF24L01].port, Transmitter.module_enable[NRF24L01].pin)
#define CS_LO() gpio_clear(Transmitter.module_enable[NRF24L01].port, Transmitter.module_enable[NRF24L01].pin)

static u8 rf_setup = 0x0F;

void NRF24L01_Initialize()
{
    rf_setup = 0x0F;
}    

u8 NRF24L01_WriteReg(u8 reg, u8 data)
{
    CS_LO();
    u8 res = spi_xfer(SPI2, W_REGISTER | (REGISTER_MASK & reg));
    spi_xfer(SPI2, data);
    CS_HI();
    return res;
}

u8 NRF24L01_WriteRegisterMulti(u8 reg, u8 data[], u8 length)
{
    CS_LO();
    u8 res = spi_xfer(SPI2, W_REGISTER | ( REGISTER_MASK & reg));
    for (u8 i = 0; i < length; i++)
    {
        spi_xfer(SPI2, data[i]);
    }
    CS_HI();
    return res;
}

u8 NRF24L01_WritePayload(u8 *data, u8 length)
{
    CS_LO();
    u8 res = spi_xfer(SPI2, W_TX_PAYLOAD);
    for (u8 i = 0; i < length; i++)
    {
        spi_xfer(SPI2, data[i]);
    }
    CS_HI();
    return res;
}

u8 NRF24L01_ReadReg(u8 reg)
{
    CS_LO();
    spi_xfer(SPI2, R_REGISTER | (REGISTER_MASK & reg));
    u8 data = spi_xfer(SPI2, 0xFF);
    CS_HI();
    return data;
}

u8 NRF24L01_ReadRegisterMulti(u8 reg, u8 data[], u8 length)
{
    CS_LO();
    u8 res = spi_xfer(SPI2, R_REGISTER | (REGISTER_MASK & reg));
    for(u8 i = 0; i < length; i++)
    {
        data[i] = spi_xfer(SPI2, 0xFF);
    }
    CS_HI();
    return res;
}

u8 NRF24L01_ReadPayload(u8 *data, u8 length)
{
    CS_LO();
    u8 res = spi_xfer(SPI2, R_RX_PAYLOAD);
    for(u8 i = 0; i < length; i++)
    {
        data[i] = spi_xfer(SPI2, 0xFF);
    }
    CS_HI();
    return res;
}

static u8 Strobe(u8 state)
{
    CS_LO();
    u8 res = spi_xfer(SPI2, state);
    CS_HI();
    return res;
}

u8 NRF24L01_FlushTx()
{
    return Strobe(FLUSH_TX);
}

u8 NRF24L01_FlushRx()
{
    return Strobe(FLUSH_RX);
}

u8 NRF24L01_Activate(u8 code)
{
    CS_LO();
    u8 res = spi_xfer(SPI2, ACTIVATE);
    spi_xfer(SPI2, code);
    CS_HI();
    return res;
}

u8 NRF24L01_SetBitrate(u8 bitrate)
{
    rf_setup = (rf_setup & 0xF7) | ((bitrate & 0x01) << 3);
    return NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
}

// Power setting is 0..3 for nRF24L01
// Claimed power amp for nRF24L01 from eBay is 20dBm. 
//      Raw            w 20dBm PA
// 0 : -18dBm  (16uW)   2dBm (1.6mW)
// 1 : -12dBm  (60uW)   8dBm   (6mW)
// 2 :  -6dBm (250uW)  14dBm  (25mW)
// 3 :   0dBm   (1mW)  20dBm (100mW)
// So it maps to Deviation as follows
/*
TXPOWER_100uW  = -10dBm
TXPOWER_300uW  = -5dBm
TXPOWER_1mW    = 0dBm
TXPOWER_3mW    = 5dBm
TXPOWER_10mW   = 10dBm
TXPOWER_30mW   = 15dBm
TXPOWER_100mW  = 20dBm
TXPOWER_150mW  = 22dBm
*/
u8 NRF24L01_SetPower(u8 power)
{
    u8 nrf_power = 0;
    switch(power) {
        case TXPOWER_100uW: nrf_power = 0; break;
        case TXPOWER_300uW: nrf_power = 0; break;
        case TXPOWER_1mW:   nrf_power = 0; break;
        case TXPOWER_3mW:   nrf_power = 1; break;
        case TXPOWER_10mW:  nrf_power = 1; break;
        case TXPOWER_30mW:  nrf_power = 2; break;
        case TXPOWER_100mW: nrf_power = 3; break;
        case TXPOWER_150mW: nrf_power = 3; break;
        default:            nrf_power = 0; break;
    };
    // Power is in range 0..3 for nRF24L01
    rf_setup = (rf_setup & 0xF9) | ((nrf_power & 0x03) << 1);
    return NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
}

#endif // defined(PROTO_HAS_NRF24L01)
