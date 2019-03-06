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
#include "common.h"
#include "devo.h"
#include "config/tx.h"
#include "protocol/interface.h"
#include <stdlib.h>
#include "target/drivers/mcu/stm32/spi.h"
#include "target/drivers/mcu/stm32/rcc.h"

#ifndef HAS_4IN1_FLASH
    #define HAS_4IN1_FLASH 0
#endif

#if HAS_MULTIMOD_SUPPORT
int SPI_ConfigSwitch(unsigned csn_high, unsigned csn_low)
{
    int i;
    //Switch output on clock before switching off SPI
    //Otherwise the pin will float which could cause a false trigger
    //SCK is now on output
    GPIO_setup_output(PROTO_SPI_CFG.sck, OTYPE_PUSHPULL);
    spi_disable(PROTO_SPI.spi);
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    GPIO_pin_set(PROTO_SPI_CFG.sck);
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    GPIO_pin_clear(PROTO_SPI_CFG.sck);
    //Switch back to SPI
    GPIO_setup_output_af(PROTO_SPI_CFG.sck, OTYPE_PUSHPULL, PROTO_SPI.spi);
    spi_set_baudrate_prescaler(PROTO_SPI.spi, SPI_CR1_BR_FPCLK_DIV_128);
    spi_enable(PROTO_SPI.spi);
    //Finally ready to send a command
    
    int byte1 = spi_xfer(PROTO_SPI.spi, csn_high);  // Set all other pins high
    int byte2 = spi_xfer(PROTO_SPI.spi, csn_low);   // Toggle related pin with CSN
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    //Reset transfer speed
    spi_disable(PROTO_SPI.spi);
    spi_set_baudrate_prescaler(PROTO_SPI.spi, PROTO_SPI_CFG.rate);
    spi_enable(PROTO_SPI.spi);
    return byte1 == 0xa5 ? byte2 : 0;
}

int SPI_ProtoGetPinConfig(int module, int state) {
    if (module >= TX_MODULE_LAST || Transmitter.module_enable[module].port != SWITCH_ADDRESS)
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
    /*
    if(state == RESET_PIN) {
        if (module == CYRF6936)
            return 1 << ((Transmitter.module_enable[module].pin >> 8) & 0x0F);
        return 0;
    }
    */
    return 0;
}
#endif

void SPI_ProtoInit()
{
// If we use SPI Switch board then SPI2 is shared between RF chips
// and flash, so it is initialized in SPIFlash.
    if (HAS_PIN(PROTO_RST_PIN)) {
        rcc_periph_clock_enable(get_rcc_from_pin(PROTO_RST_PIN));
        GPIO_setup_output(PROTO_RST_PIN, OTYPE_PUSHPULL);
        GPIO_pin_set(PROTO_RST_PIN);
        usleep(10);
        GPIO_pin_clear(PROTO_RST_PIN);
    }
    if (PROTO_SPI_CFG.spi != FLASH_SPI_CFG.spi) {
        _spi_init(PROTO_SPI_CFG);
        if (HAS_4IN1_FLASH) {
            SPISwitch_Init();
        }
    }

#if HAS_MULTIMOD_SUPPORT
    if(Transmitter.module_enable[MULTIMOD].port) {
        struct mcu_pin port = Transmitter.module_enable[MULTIMOD];
        printf("Switch port: %08x pin: %04x\n", port.port, port.pin);
        GPIO_setup_output(port, OTYPE_PUSHPULL);
        GPIO_pin_set(port);
    }
#endif //HAS_MULTIMOD_SUPPORT
    for (int i = 0; i < MULTIMOD; i++) {
        if(Transmitter.module_enable[i].port
           && Transmitter.module_enable[i].port != SWITCH_ADDRESS)
        {
            struct mcu_pin port = Transmitter.module_enable[i];
            printf("%s port: %08x pin: %04x\n", MODULE_NAME[i], port.port, port.pin);
            GPIO_setup_output(port, OTYPE_PUSHPULL);
            GPIO_pin_set(port);
        }
    }
}

void SPI_AVRProgramInit()
{
    rcc_set_ppre1(RCC_CFGR_PPRE1_HCLK_DIV16);  // 72 / 16 = 4.5MHz
    spi_disable(PROTO_SPI.spi);
    spi_set_baudrate_prescaler(PROTO_SPI.spi, SPI_CR1_BR_FPCLK_DIV_256);  // 4.5 / 256 = 17.5kHz
    spi_enable(PROTO_SPI.spi);
}

void MCU_InitModules()
{
    //CSN                               
    GPIO_setup_output(PROTO_SPI.csn, OTYPE_PUSHPULL);
    GPIO_pin_set(PROTO_SPI.csn);
    Transmitter.module_enable[CYRF6936] = PROTO_SPI.csn;
    Transmitter.module_poweramp = (1 << CYRF6936);
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
#if HAS_MULTIMOD_SUPPORT
        case 'S':
        case 's':
            port->port = SWITCH_ADDRESS; break;
#endif
        default:
            port->port = 0;
            port->pin = 0;
            if(strcasecmp(name, "None") == 0) {
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
