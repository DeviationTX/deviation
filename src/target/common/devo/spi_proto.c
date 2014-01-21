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
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "devo.h"
#include "config/tx.h"
#include "protocol/interface.h"
#include <stdlib.h>

void SPI_ConfigSwitch(unsigned csn_high, unsigned csn_low)
{
    int i;
    spi_disable(SPI2);
    //SCK is now on output
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    gpio_clear(GPIOB, GPIO13); // It should already be clear, but whatever
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    gpio_set(GPIOB, GPIO13);
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    gpio_clear(GPIOB, GPIO13);
    //Switch back to SPI
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO13);
    spi_set_baudrate_prescaler(SPI2, SPI_CR1_BR_FPCLK_DIV_128);
    spi_enable(SPI2);
    //Finally ready to send a command
    
    spi_xfer(SPI2, csn_high); //Set all other pins high
    spi_xfer(SPI2, csn_low); //Toggle related pin with CSN
    for(i = 0; i < 100; i++)
        asm volatile ("nop");
    //Reset transfer speed
    spi_disable(SPI2);
    spi_set_baudrate_prescaler(SPI2, SPI_CR1_BR_FPCLK_DIV_16);
    spi_enable(SPI2);
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
    if(state == RESET_PIN) {
        if (module == CYRF6936)
            return 1 << ((Transmitter.module_enable[module].pin >> 8) & 0x0F);
        return 0;
    }
    return 0;
}

void SPI_ProtoInit()
{
    const char * const modules[PROGSWITCH] = {
        [CYRF6936] = "CYRF",
        [A7105]    = "A7105",
        [CC2500]   = "CC2500",
        [NRF24L01] = "nRF24L01",
    };

    /* Enable SPI2 */
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_SPI2EN);
    /* Enable GPIOA */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    /* Enable GPIOB */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);

    /* SCK, MOSI */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO13 | GPIO15);
    /* MISO */
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO14);

    /*CYRF cfg */
    /* Reset and CS */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO11);
    gpio_clear(GPIOB, GPIO11);

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

    if(Transmitter.module_enable[PROGSWITCH].port) {
        struct mcu_pin *port = &Transmitter.module_enable[PROGSWITCH];
        printf("Switch port: %08x pin: %04x\n", port->port, port->pin);
        gpio_set_mode(port->port, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, port->pin);
        gpio_set(port->port, port->pin);
    }
    for (int i = 0; i < PROGSWITCH; i++) {
        if(Transmitter.module_enable[i].port
           && Transmitter.module_enable[i].port != SWITCH_ADDRESS)
        {
            struct mcu_pin *port = &Transmitter.module_enable[i];
            printf("%s port: %08x pin: %04x\n", modules[i], port->port, port->pin);
            gpio_set_mode(port->port, GPIO_MODE_OUTPUT_50_MHZ,
                      GPIO_CNF_OUTPUT_PUSHPULL, port->pin);
            gpio_set(port->port, port->pin);
        }
    }
    /* Includes enable? */
    spi_init_master(SPI2, 
                    SPI_CR1_BAUDRATE_FPCLK_DIV_16,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, 
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI2);
    spi_set_nss_high(SPI2);
    spi_enable(SPI2);

#ifndef MODULAR
    //Need to figure out how to do this given the protocols are loaded as modules
    //We want to set up modules for low-power mode
    if (Transmitter.module_enable[A7105].port) {
        PROTOCOL_SetSwitch(A7105);
        A7105_SetTxRxMode(TXRX_OFF);
    }
    if (Transmitter.module_enable[CC2500].port) {
        PROTOCOL_SetSwitch(CC2500);
        CC2500_SetTxRxMode(TXRX_OFF);
    }
#endif
    PROTO_Stubs(0);
}

void MCU_InitModules()
{
    Transmitter.module_enable[CYRF6936].port = GPIOB;
    Transmitter.module_enable[CYRF6936].pin = GPIO12;
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

const char *MCU_GetPinName(char *str, struct mcu_pin *port)
{
/*
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
*/
    if (port->port == SWITCH_ADDRESS) {
        sprintf(str, "S%4x", port->pin);
        return str;
    } else {
        str[0] = 'A' + ((port->port - GPIOA) / (GPIOB - GPIOA));
        if (str[0] > 'G')
            return "None";
    }
    for(int i = 0; i < 16; i++) {
        if(port->pin == (1 << i)) {
            sprintf(str+1, "%d", i);
            return str;
        }
    }
    return "None";
}
