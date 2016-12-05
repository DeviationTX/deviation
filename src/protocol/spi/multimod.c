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
#include "config/tx.h"
#include "protocol/interface.h"
#include "protospi.h"
#define IS_UNIVERSALTX() (Transmitter.module_enable[MULTIMODCTL].port != 0)

#if HAS_MULTIMOD_SUPPORT
static void utx_SendReceive(u8 *tx, u8 *rx, int len)
{
    //spi_set_baudrate_prescaler(SPI2, SPI_CR1_BR_FPCLK_DIV_128);
    PROTOSPI_pin_clear(Transmitter.module_enable[MULTIMODCTL]);
    for(int i = 0; i < 50; i++)
        _NOP();
    for (int i = 0; i < len; i++) {
        rx[i] = PROTOSPI_xfer(tx[i]);
    }
    PROTOSPI_pin_set(Transmitter.module_enable[MULTIMODCTL]);
    for(int i = 0; i < 100; i++)
        _NOP();
    //spi_set_baudrate_prescaler(SPI2, SPI_CR1_BR_FPCLK_DIV_16);
}

static int utx_SwitchCommand(int module, int command)
{
    int data_len = 1;
    u8 tx[4];
    u8 rx[4];
    tx[0] = command;
    switch (command) {
        case SET_PIN_ENABLE:
        case CLEAR_PIN_ENABLE:
        case TXRX_OFF:
        case TX_EN:
        case RX_EN:
            break;
        case CHANGE_MODULE:
            tx[1] = module;
            data_len = 2;
            break;
    }
    utx_SendReceive(tx, rx, data_len);
    return (rx[0] == 0x5a);
}

static int mm_ProtoGetPinConfig(int module, int state) {
    if (module >= TX_MODULE_LAST || Transmitter.module_enable[module].port != SWITCH_ADDRESS)
        return 0;
    if(state == CSN_PIN)
        return 1 << (Transmitter.module_enable[module].pin & 0x0F);
    if(state == ENABLED_PIN) {
        if(module == NRF24L01) {
            return 1 << ((Transmitter.module_enable[module].pin >> 8) & 0x0F);
#ifdef PROTO_HAS_CYRF6936
        } else if (module == CYRF6936 && (Transmitter.module_enable[module].pin >> 8) == CYRF6936_AWA24S) {
            return 0x80;
#endif
        }
        return 0;
    }
    /*
    if(state == RESET_PIN) {
        if (module == CYRF6936)
            return 1 << ((Transmitter.module_enable[module].pin >> 8) & 0x0F);
        return 0;
    }
    */
    return 0;
}

static int mm_SwitchCommand(int module, int command)
{
    u8 static_val = 0;
    switch (command) {
        case SET_PIN_ENABLE:
            static_val = mm_ProtoGetPinConfig(module, ENABLED_PIN);
            break;
        case CLEAR_PIN_ENABLE:
        case CHANGE_MODULE:
            break;
        case TXRX_OFF:
        case TX_EN:
        case RX_EN:
            if (module == CYRF6936 && Transmitter.module_enable[CYRF6936].port == SWITCH_ADDRESS) {
                u8 en = mm_ProtoGetPinConfig(module, ENABLED_PIN);
                if (en) {
                    static_val = en;
                    break;
                }
            }
            return 0;
        default:
            return 0;
    }
    u8 csn = mm_ProtoGetPinConfig(module, CSN_PIN);
    u8 csn_high = static_val | 0x0f;
    u8 csn_low  = static_val | (0x0f ^ csn);
#if 0
    SPI_SwitchStartData();
    int byte1 = PROTOSPI_xfer(csn_high);
    int byte2 = PROTOSPI_xfer(csn_low);
    SPI_SwitchStopData();
    return (command == CHANGE_MODULE && byte1 == 0xa5) ? byte2 : 0;
#else
    return SPI_ConfigSwitch(csn_high, csn_low);
#endif
}

int MULTIMOD_SwitchCommand(int module, int command)
{
    if(!Transmitter.module_enable[MULTIMOD].port || (module < MULTIMOD && Transmitter.module_enable[module].port != SWITCH_ADDRESS)) {
        return 0;
    }
    if(IS_UNIVERSALTX()) {
        return utx_SwitchCommand(module, command);
    } else {
        return mm_SwitchCommand(module, command);
    }
}

#endif //HAS_MULTIMOD_SUPPORT
void MODULE_CSN(int module, int set)
{
#if HAS_MULTIMOD_SUPPORT
    if (Transmitter.module_enable[MULTIMOD].port) {
         if (!IS_UNIVERSALTX() || Transmitter.module_enable[module].port == SWITCH_ADDRESS) {
            //We need to set the multimodule CSN even if we don't use it
            //for this protocol so that it doesn't interpret commands
            //UniversalTx doesn't need that since it has a separate CSN pin for control logic
            if (set) {
                PROTOSPI_pin_set(Transmitter.module_enable[MULTIMOD]);
            } else {
                PROTOSPI_pin_clear(Transmitter.module_enable[MULTIMOD]);
            }
            if(Transmitter.module_enable[module].port == SWITCH_ADDRESS) {
                for(int i = 0; i < 20; i++)
                    _NOP();
                return;
            }
        }
    }
#endif
    if (set) {
        PROTOSPI_pin_set(Transmitter.module_enable[module]);
    } else {
        PROTOSPI_pin_clear(Transmitter.module_enable[module]);
    }
}
