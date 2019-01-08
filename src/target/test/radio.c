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
#include "config/model.h"
#include "config/tx.h"

#include <stdlib.h>

#define GPIOA 0xAAAAAAAA
#define GPIOB 0xBBBBBBBB
#define GPIOC 0xCCCCCCCC
#define GPIOD 0xDDDDDDDD
#define GPIOE 0xEEEEEEEE
#define GPIOF 0xFFFFFFFF
#define GPIOG 0xABCDEFAB
void SPI_ProtoInit() {}
void MCU_InitModules() {
    Transmitter.module_enable[CYRF6936].port = GPIOB;
    Transmitter.module_enable[CYRF6936].pin  = 1 << 12;
    Transmitter.module_poweramp = 1;
};
int MCU_SetPin(struct mcu_pin *port, const char *name) {
    switch(name[0]) {
        case 'A':
        case 'a':
            port->port = GPIOA; break;
        case 'B':
        case 'b':
            port->port = GPIOB; break;
        case 'C':
        case 'c':
            port->port = GPIOC; break;
        case 'D':
        case 'd':
            port->port = GPIOD; break;
        case 'E':
        case 'e':
            port->port = GPIOE; break;
        case 'F':
        case 'f':
            port->port = GPIOF; break;
        case 'G':
        case 'g':
            port->port = GPIOG; break;
        case 'S':
        case 's':
            port->port = SWITCH_ADDRESS; break;
        default:
            if(strcasecmp(name, "None") == 0) {
                port->port = 0;
                port->pin = 0;
                return 1;
            }
            return 0;
    }
    if (port->port != SWITCH_ADDRESS) {
        int x = atoi(name+1);
        if (x > 15)
            return 0;
        port->pin = 1 << x;
    } else {
        port->pin = strtol(name+1,NULL, 16);
    }
    if(port == &Transmitter.module_enable[0]) {
        printf("Set CYRF6936 Enable: %s ", name);
    } else if(port == &Transmitter.module_enable[1]) {
        printf("Set A7105 Enable: %s ", name);
    } else if(port == &Transmitter.module_enable[2]) {
        printf("Set CC2500 Enable: %s ", name);
    } else if(port == &Transmitter.module_enable[3]) {
        printf("Set NRF24L01 Enable: %s ", name);
    } else {
        return 0;
    }
    printf("port: %08x pin: %04x\n", port->port, port->pin);
    return 1;
}

const char *MCU_GetPinName(char *str, struct mcu_pin *port)
{
    switch(port->port) {
        case GPIOA: str[0] = 'A'; break;
        case GPIOB: str[0] = 'B'; break;
        case GPIOC: str[0] = 'C'; break;
        case GPIOD: str[0] = 'D'; break;
        case GPIOE: str[0] = 'E'; break;
        case GPIOF: str[0] = 'F'; break;
        case GPIOG: str[0] = 'G'; break;
        default: return "None"; break;
    }
    for(int i = 0; i < 16; i++) {
        if(port->pin == (1uL << i)) {
            sprintf(str+1, "%d", i);
            return str;
        }
    }
    return "None";
}

int SPI_ConfigSwitch(unsigned csn_high, unsigned csn_low) {
    (void)csn_high;
    (void)csn_low;
    return 1;
}

int SPI_ProtoGetPinConfig(int module, int state) {
    (void)module;
    (void)state;
    return 0;
}

u8 PROTOSPI_read3wire() { return 0x00; }

u8 PROTOSPI_xfer(u8 byte) { return byte; }

#ifdef PROTO_HAS_A7105
int A7105_Reset() { return 1; }
#endif

#ifdef PROTO_HAS_CC2500
int CC2500_Reset() { return 1;}
#endif //PROTO_HAS_CC2500
/* CYRF */
int CYRF_Reset() {return 1;}

void SPI_AVRProgramInit() {}
void PWM_Initialize() {}
void PWM_Stop() {}
void PPM_Enable(unsigned low_time, volatile u16 *pulses) {
    int i;
    printf("PPM: low=%d ", (int)low_time);
    for(i = 0; pulses[i]; i++)
        printf("%04d ", pulses[i]);
    printf("\n");
}
