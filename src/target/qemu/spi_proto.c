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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
//#include "../common/devo/devo.h"
#include "config/tx.h"
//#include "protocol/interface.h"
//#include <stdlib.h>


void SPI_ProtoInit()
{
// If we use SPI Switch board then SPI2 is shared between RF chips
// and flash, so it is initialized in SPIFlash.
//#if !defined HAS_4IN1_FLASH || !HAS_4IN1_FLASH
#if _SPI_PROTO_PORT != _SPI_FLASH_PORT
    #if _SPI_PROTO_PORT == 1
        #define SPIx        SPI1
        #define SPIxEN      RCC_APB2ENR_SPI1EN
        #define APB_SPIxEN  RCC_APB2ENR
    #elif _SPI_PROTO_PORT == 2
        #define SPIx        SPI2
        #define SPIxEN      RCC_APB1ENR_SPI2EN
        #define APB_SPIxEN  RCC_APB1ENR
    #endif
    /* Enable SPIx */
    rcc_peripheral_enable_clock(&APB_SPIxEN,  SPIxEN);
    /* Enable GPIOA */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    /* Enable GPIOB */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);

    PORT_mode_setup(PROTO_RST_PIN,  GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    PORT_mode_setup(PROTO_CSN_PIN,  GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    PORT_mode_setup(PROTO_SCK_PIN,  GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL);
    PORT_mode_setup(PROTO_MOSI_PIN, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL);
    PORT_mode_setup(PROTO_MISO_PIN, GPIO_MODE_INPUT,         GPIO_CNF_INPUT_FLOAT);
    
    PORT_pin_clear(PROTO_RST_PIN);
    PORT_pin_set(PROTO_CSN_PIN);


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
}

void MCU_InitModules()
{
}

int MCU_SetPin(struct mcu_pin *port, const char *name) {
//    (void)port;
    (void)name;
    port->port = 1;
    return 1;
}
