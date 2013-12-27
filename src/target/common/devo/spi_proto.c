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
#include <stdlib.h>

void SPI_ProtoInit()
{
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
    if(Transmitter.module_enable[CYRF6936].port) {
        struct mcu_pin *port = &Transmitter.module_enable[CYRF6936];
        printf("CYRF port: %08x pin: %04x\n", port->port, port->pin);
        //GPIOB.12
        gpio_set_mode(Transmitter.module_enable[CYRF6936].port, GPIO_MODE_OUTPUT_50_MHZ,
                      GPIO_CNF_OUTPUT_PUSHPULL, Transmitter.module_enable[CYRF6936].pin);
        gpio_set(Transmitter.module_enable[CYRF6936].port, Transmitter.module_enable[CYRF6936].pin);
    }

    //Disable JTAG and SWD
    AFIO_MAPR = (AFIO_MAPR & ~AFIO_MAPR_SWJ_MASK) | AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF;

    /* A7105 */
#ifdef PROTO_HAS_A7105
    if(Transmitter.module_enable[A7105].port) {
        //GPIOA.13
        struct mcu_pin *port = &Transmitter.module_enable[A7105];
        printf("A7105 port: %08x pin: %04x\n", port->port, port->pin);
        gpio_set_mode(Transmitter.module_enable[A7105].port, GPIO_MODE_OUTPUT_50_MHZ,
                      GPIO_CNF_OUTPUT_PUSHPULL, Transmitter.module_enable[A7105].pin);
        gpio_set(Transmitter.module_enable[A7105].port, Transmitter.module_enable[A7105].pin);
    }
#endif

    /* CC2500 */
#ifdef PROTO_HAS_CC2500
    if(Transmitter.module_enable[CC2500].port) {
        //GPIOA.14
        struct mcu_pin *port = &Transmitter.module_enable[CC2500];
        printf("CC2500 port: %08x pin: %04x\n", port->port, port->pin);
        gpio_set_mode(Transmitter.module_enable[CC2500].port, GPIO_MODE_OUTPUT_50_MHZ,
                      GPIO_CNF_OUTPUT_PUSHPULL, Transmitter.module_enable[CC2500].pin);
        gpio_set(Transmitter.module_enable[CC2500].port, Transmitter.module_enable[CC2500].pin);
    }
#endif
    /* NRF24L01 */
#ifdef PROTO_HAS_NRF24L01
    if(Transmitter.module_enable[NRF24L01].port) {
        //GPIOA.14
        struct mcu_pin *port = &Transmitter.module_enable[NRF24L01];
        printf("nRF24L01 port: %08x pin: %04x\n", port->port, port->pin);
        gpio_set_mode(Transmitter.module_enable[NRF24L01].port, GPIO_MODE_OUTPUT_50_MHZ,
                      GPIO_CNF_OUTPUT_PUSHPULL, Transmitter.module_enable[NRF24L01].pin);
        gpio_set(Transmitter.module_enable[NRF24L01].port, Transmitter.module_enable[NRF24L01].pin);
    }
#endif
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
        default:
            if(strcasecmp(name, "None") == 0) {
                port->port = 0;
                port->pin = 0;
                return 1;
            }
            return 0;
    }
    int x = atoi(name+1);
    if (x > 15)
        return 0;
    port->pin = 1 << x;
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
    str[0] = 'A' + ((port->port - GPIOA) / (GPIOB - GPIOA));
    if (str[0] > 'G')
        return "None";
    for(int i = 0; i < 16; i++) {
        if(port->pin == (1 << i)) {
            sprintf(str+1, "%d", i);
            return str;
        }
    }
    return "None";
}
