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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "devo.h"
#include "config/tx.h"
#include "protocol/interface.h"
#include <stdlib.h>

#if _SPI_PROTO_PORT == 1
    #define SPIx        SPI1
    #define SPIxEN      RCC_APB2ENR_SPI1EN
    #define APB_SPIxEN  RCC_APB2ENR
#elif _SPI_PROTO_PORT == 2
    #define SPIx        SPI2
    #define SPIxEN      RCC_APB1ENR_SPI2EN
    #define APB_SPIxEN  RCC_APB1ENR
#endif

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
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    spi_disable(SPIx);
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    gpio_set(GPIOB, GPIO13);
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    gpio_clear(GPIOB, GPIO13);
    //Switch back to SPI
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO13);
    spi_set_baudrate_prescaler(SPIx, SPI_CR1_BR_FPCLK_DIV_128);
    spi_enable(SPIx);
    //Finally ready to send a command
    
    int byte1 = spi_xfer(SPIx, csn_high); //Set all other pins high
    int byte2 = spi_xfer(SPIx, csn_low); //Toggle related pin with CSN
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    //Reset transfer speed
    spi_disable(SPIx);
    spi_set_baudrate_prescaler(SPIx, SPI_CR1_BR_FPCLK_DIV_16);
    spi_enable(SPIx);
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
#if _SPI_PROTO_PORT != _SPI_FLASH_PORT
    /* Enable SPIx */
    rcc_peripheral_enable_clock(&APB_SPIxEN, SPIxEN);
    /* Enable GPIOA */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    /* Enable GPIOB */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);

    PORT_mode_setup(PROTO_RST_PIN,  GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    PORT_mode_setup(PROTO_SCK_PIN,  GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL);
    PORT_mode_setup(PROTO_MOSI_PIN, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL);
    PORT_mode_setup(PROTO_MISO_PIN, GPIO_MODE_INPUT,         GPIO_CNF_INPUT_FLOAT);
    
    PORT_pin_clear(PROTO_RST_PIN);

#if 0 //In power.c
    //Disable JTAG and SWD and set both pins high
    AFIO_MAPR = (AFIO_MAPR & ~AFIO_MAPR_SWJ_MASK) | AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF;
    gpio_set_mode(GPIO_BANK_JTMS_SWDIO, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO_JTMS_SWDIO);
    gpio_set(GPIO_BANK_JTMS_SWDIO, GPIO_JTMS_SWDIO);
    gpio_set_mode(GPIO_BANK_JTCK_SWCLK, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO_JTCK_SWCLK);
    gpio_set(GPIO_BANK_JTCK_SWCLK, GPIO_JTCK_SWCLK);
#endif

    /* Includes enable? */
    spi_init_master(SPIx, 
                    SPI_CR1_BAUDRATE_FPCLK_DIV_16,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, 
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPIx);
    spi_set_nss_high(SPIx);
    spi_enable(SPIx);
#endif

    if (HAS_4IN1_FLASH && _SPI_FLASH_PORT != _SPI_PROTO_PORT) {
        SPISwitch_Init();
    }

#if HAS_MULTIMOD_SUPPORT
    if(Transmitter.module_enable[MULTIMOD].port) {
        struct mcu_pin *port = &Transmitter.module_enable[MULTIMOD];
        printf("Switch port: %08x pin: %04x\n", port->port, port->pin);
        gpio_set_mode(port->port, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, port->pin);
        gpio_set(port->port, port->pin);
    }
#endif //HAS_MULTIMOD_SUPPORT
    for (int i = 0; i < MULTIMOD; i++) {
        if(Transmitter.module_enable[i].port
           && Transmitter.module_enable[i].port != SWITCH_ADDRESS)
        {
            struct mcu_pin *port = &Transmitter.module_enable[i];
            printf("%s port: %08x pin: %04x\n", MODULE_NAME[i], port->port, port->pin);
            gpio_set_mode(port->port, GPIO_MODE_OUTPUT_50_MHZ,
                      GPIO_CNF_OUTPUT_PUSHPULL, port->pin);
            gpio_set(port->port, port->pin);
        }
    }
    PROTO_Stubs(0);
}

void SPI_AVRProgramInit()
{
    rcc_set_ppre1(RCC_CFGR_PPRE1_HCLK_DIV16);  //72 / 16 = 4.5MHz
    spi_disable(SPIx);
    spi_set_baudrate_prescaler(SPIx, SPI_CR1_BR_FPCLK_DIV_256);// 4.5 / 256 = 17.5kHz
    spi_enable(SPIx);
}

void MCU_InitModules()
{
    //CSN                               
    PORT_mode_setup(PROTO_CSN_PIN,  GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    PORT_pin_set(PROTO_CSN_PIN);
    Transmitter.module_enable[CYRF6936] = PROTO_CSN_PIN;
    //Transmitter.module_enable[CYRF6936].port = GPIOB;
    //Transmitter.module_enable[CYRF6936].pin = GPIO12;
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
