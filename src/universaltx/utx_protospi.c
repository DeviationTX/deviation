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
#include "common.h"
#include "protospi.h"

uint8_t spi_xfer8(uint32_t spi, uint8_t data)
{
	SPI_DR8(spi) = data;

	/* Wait for transfer finished. */
	while (!(SPI_SR(spi) & SPI_SR_RXNE));

	/* Read the data (8 or 16 bits, depending on DFF bit) from DR. */
	return SPI_DR8(spi);
}

u8 PROTOSPI_read3wire(){
    u8 data;
    while(!(SPI_SR(SPI2) & SPI_SR_TXE))
        ;
    while((SPI_SR(SPI2) & SPI_SR_BSY))
        ;

    spi_disable(SPI2);
    spi_set_bidirectional_receive_only_mode(SPI2);
    /* Force read from SPI_DR to ensure RXNE is clear (probably not needed) */
    volatile u8 x = SPI_DR8(SPI2);
    (void)x;
    spi_enable(SPI2);  //This starts the data read
    //Wait > 1 SPI clock (but less than 8).  clock is 4.5MHz
    asm volatile ("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
                  "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
                  "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
                  "nop\n\tnop\n\tnop\n\tnop\n\tnop");
    spi_disable(SPI2); //This ends the read window
    data = spi_read8(SPI2);
    spi_set_unidirectional_mode(SPI2);
    spi_enable(SPI2);
    return data;
}
void SPI_ProtoInit()
{
#ifdef DISCOVERY
    /* This is only relavent to the Discovery board
       It uses SPI2 to control the MEMS chip, which interferest with theUniversalTx operation
       CSN is on PC.0, so pull it high to ensure the MEMS device is disabled
     */
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);
    gpio_set(GPIOC, GPIO0);
#endif


    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_SPI2);
    PROTOSPI_mode_setup(module_enable[A7105]    , GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PROTOSPI_mode_setup(module_enable[CYRF6936] , GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PROTOSPI_mode_setup(module_enable[CC2500]   , GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PROTOSPI_mode_setup(module_enable[NRF24L01] , GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PROTOSPI_mode_setup(NRF24L01_CE             , GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);

    PROTOSPI_mode_setup(RF_MUXSEL1, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PROTOSPI_mode_setup(RF_MUXSEL2, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);

    PROTOSPI_mode_setup(INPUT_CSN , GPIO_MODE_INPUT, GPIO_PUPD_PULLUP);
   
    PROTOSPI_mode_setup(SCK,        GPIO_MODE_AF, GPIO_PUPD_NONE); 
    PROTOSPI_mode_setup(MOSI,       GPIO_MODE_AF, GPIO_PUPD_NONE); 
    PROTOSPI_mode_setup(MISO,       GPIO_MODE_AF, GPIO_PUPD_NONE); 
    gpio_set_af(GPIOB, GPIO_AF0, SCK.pin | MOSI.pin | MISO.pin);

    PROTOSPI_pin_set(module_enable[A7105]);
    PROTOSPI_pin_set(module_enable[CYRF6936]);
    PROTOSPI_pin_set(module_enable[CC2500]);
    PROTOSPI_pin_set(module_enable[NRF24L01]);
    PROTOSPI_pin_clear(NRF24L01_CE);
    PROTOSPI_pin_clear(RF_MUXSEL1);
    PROTOSPI_pin_clear(RF_MUXSEL2);

    spi_reset(SPI2);
    spi_init_master(SPI2, 
                    SPI_CR1_BAUDRATE_FPCLK_DIV_16,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, 
                    SPI_CR1_CRCL_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI2);
    spi_set_nss_high(SPI2);
    SPI_CR2(SPI2) = 0x1700;
    spi_enable(SPI2);
}
