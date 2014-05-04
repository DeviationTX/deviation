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

/*The following will force the loading of various
  functions used in the protocol modules, but unused elsewhere
  in Deviation.
  Note that we lie aboiut the arguments to these functions. It is
  Important that the actual functions never execute
*/
#include <libopencm3/stm32/f2/rcc.h>
#include <libopencm3/stm32/f2/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "config/tx.h"
#include "protocol/interface.h"
#include "protospi.h"
#include <stdlib.h>

static const struct mcu_pin SCK  = {GPIOA, GPIO14};
static const struct mcu_pin MOSI = {GPIOA, GPIO7};
static const struct mcu_pin MISO = {GPIOD, GPIO5};

void SPI_Test()
{
    static const struct mcu_pin pins[] = {
        {GPIOA, GPIO7},
        {GPIOA, GPIO13},
        {GPIOA, GPIO14},
        {GPIOD, GPIO4},
        {GPIOC, GPIO7},
        {GPIOD, GPIO5},
        {GPIOD, GPIO6},
    };
    for(unsigned int i = 0; i < (sizeof(pins) / sizeof(struct mcu_pin)); i++) {
        gpio_mode_setup(pins[i].port, GPIO_MODE_OUTPUT,  GPIO_PUPD_PULLDOWN, pins[i].pin);
    }
    for(unsigned int i = 0; i < (sizeof(pins) / sizeof(struct mcu_pin)); i++) {
        for(unsigned int j = 0; j < i+1; j++) {
            PROTOSPI_pin_set(pins[i]);
            usleep(50);
            PROTOSPI_pin_clear(pins[i]);
            usleep(50);
        }
    }
} 
void _SPI_DELAY()
{
    for(int i = 0; i < 20; i++)
        _NOP();
}
u8 PROTOSPI_read3wire()
{
    u8 rx = 0;
    int i=8;
    gpio_mode_setup(MOSI.port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, MOSI.pin);

    while (i--) {
        PROTOSPI_pin_clear(SCK);
        _SPI_DELAY();
        rx <<= 1;
        rx |= gpio_get(MOSI.port, MOSI.pin) ? 1 : 0;
        PROTOSPI_pin_set(SCK);
        _SPI_DELAY();
    }
    PROTOSPI_pin_clear(SCK);
    gpio_mode_setup(MOSI.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MOSI.pin);
    return rx;
}

u8 PROTOSPI_xfer(u8 tx)
{
    int i=8;
    u8 rx = 0;

    while (i--) {
        PROTOSPI_pin_clear(SCK);
        if (tx & 0x80)
            PROTOSPI_pin_set(MOSI);
        else
            PROTOSPI_pin_clear(MOSI);
        _SPI_DELAY();
        rx <<= 1;
        tx <<= 1;
        rx |= gpio_get(MISO.port, MISO.pin) ? 1 : 0;
        PROTOSPI_pin_set(SCK);
        _SPI_DELAY();
    }
    PROTOSPI_pin_clear(SCK);
    return rx;
}

#if HAS_MULTIMOD_SUPPORT
int SPI_ConfigSwitch(unsigned csn_high, unsigned csn_low)
{
    int i;
    //Switch output on clock before switching off SPI
    //Otherwise the pin will float which could cause a false trigger
    //SCK is now on output
    PROTOSPI_pin_clear(SCK);
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    PROTOSPI_pin_set(SCK);
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    PROTOSPI_pin_clear(SCK);
    //Finally ready to send a command
    int byte1 = PROTOSPI_xfer(csn_high);  //Set all other pins high
    int byte2 = PROTOSPI_xfer(csn_low);   //Toggle related pin with CSN
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    return byte1 == 0xa5 ? byte2 : 0;
}

int SPI_ProtoGetPinConfig(int module, int state) {
    if (Transmitter.module_enable[module].port != SWITCH_ADDRESS)
        return 0;
    if(state == CSN_PIN)
        return 1 << (Transmitter.module_enable[module].pin & 0x0F);
    if(state == ENABLED_PIN) {
        if(module == NRF24L01) {
            return 1 << ((Transmitter.module_enable[module].pin >> 8) & 0x0F);
        }
        return 0;
    }
    if(state == DISABLED_PIN) {
        return 0;
    }
    return 0;
}
#endif

void SPI_ProtoInit()
{
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPAEN);

    gpio_mode_setup(SCK.port,  GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,   SCK.pin);
    gpio_mode_setup(MOSI.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,   MOSI.pin);
    gpio_mode_setup(MISO.port, GPIO_MODE_INPUT,  GPIO_PUPD_PULLDOWN, MISO.pin);


#if HAS_MULTIMOD_SUPPORT
    if(Transmitter.module_enable[MULTIMOD].port) {
        struct mcu_pin *port = &Transmitter.module_enable[MULTIMOD];
        printf("Switch port: %08x pin: %04x\n", port->port, port->pin);
        gpio_mode_setup(port->port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, port->pin);
        PROTOSPI_pin_set(*port);
    }
#endif //HAS_MULTIMOD_SUPPORT
    for (int i = 0; i < MULTIMOD; i++) {
        if(Transmitter.module_enable[i].port
           && Transmitter.module_enable[i].port != SWITCH_ADDRESS)
        {
            struct mcu_pin *port = &Transmitter.module_enable[i];
            printf("%s port: %08x pin: %04x\n", MODULE_NAME[i], port->port, port->pin);
            gpio_mode_setup(port->port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, port->pin);
            PROTOSPI_pin_set(*port);
        }
    }
}

void MCU_InitModules()
{
}

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
    printf("port: %08x pin: %04x\n", port->port, port->pin);
    return 1;
}
