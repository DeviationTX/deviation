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
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/dma.h>
#include "common.h"
#include "protospi.h"
#include "config/model.h"
#include "protocol/interface.h"

//#include "stm31f05xxx.h"
// clear/set macros 

//These dont' work because a 'static const' is not actually a constant in C
//ctassert(! (INPUT_CSN.pin & 0x07), INPUT_CSN_must_be_attached_to_pins4_to_15);
//ctassert(! (PASSTHRU_CSN.pin & 0x07), PASSTHRU_CSN_must_be_attached_to_pins4_to_15);
ctassert(! (PASSTHRU_CSN_PIN & 0x07), PASSTHRU_CSN_must_be_attached_to_pins4_to_15);

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


void SPI_ProtoMasterSlaveInit(u8 *read_buffer)
{
    spi_disable(SPI2);
    spi_reset(SPI2);

#if 0
        nvic_set_priority(NVIC_EXTI4_15_IRQ, PRIORITY_HIGHEST);
        nvic_enable_irq(NVIC_EXTI4_15_IRQ);
        //Setup EXTI8 (INPUT_CSN) to trigger interrupt
        exti_select_source(INPUT_CSN.pin, INPUT_CSN.port);
        exti_set_trigger(INPUT_CSN.pin, EXTI_TRIGGER_BOTH);
        exti_enable_request(INPUT_CSN.pin);
        //Setup EXTI12 (PASSTHROUGH_CSN) to trigger interrupt
        exti_select_source(PASSTHRU_CSN.pin, PASSTHRU_CSN.port);
        exti_set_trigger(PASSTHRU_CSN.pin, EXTI_TRIGGER_BOTH);
        exti_enable_request(PASSTHRU_CSN.pin);

	// Turn on SPI2
	SET_BIT(RCC_APB1ENR,BIT14);
	// Take SPI2 out of reset
	CLEAR_BIT(RCC_APB1RSTR,BIT14);
	// Set the baud rate (500kHz - assumes 8MHz pclk) not sure this has any effect in slave mode
	SET_BIT(SPI2_CR1,BIT4);
	// Set softawre NSS pin high
	SET_BIT(SPI2_CR1,BIT8);
	// Set softawre NSS
	SET_BIT(SPI2_CR1,BIT9);
	// Set the word size (8 bits)
	SET_BIT(SPI2_CR2,(15 << 8));	
	// Enable RX interrupts
	SET_BIT(SPI2_CR2,BIT6);	
	// Enable error handling interrupt
	SET_BIT(SPI2_CR2,BIT5);
        if(SPI2_SR) {*sendptr++ = 'I';}
	// enable spi
	SET_BIT(SPI2_CR1,BIT6);	
	// Enable SPI2 interrupts in NVIC
	ISER |= BIT26;
	SPI2_DR = 0x5a;
#else
    spi_init_master(SPI2, 
                    SPI_CR1_BAUDRATE_FPCLK_DIV_16,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, 
                    SPI_CR1_CRCL_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI2);
    if (read_buffer) {
        spi_set_slave_mode(SPI2);
        spi_set_nss_high(SPI2);  //Wait for 1st low transition so we don't capture half a packet

	/* SPI2 RX on DMA1 Channel 4 */
	dma_channel_reset(DMA1, DMA_CHANNEL4);
	dma_set_peripheral_address(DMA1, DMA_CHANNEL4, (uint32_t)&SPI2_DR);
	dma_set_memory_address(DMA1, DMA_CHANNEL4, (uint32_t)read_buffer);
	dma_set_read_from_peripheral(DMA1, DMA_CHANNEL4);
	dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL4);
	dma_set_peripheral_size(DMA1, DMA_CHANNEL4, DMA_CCR_PSIZE_8BIT);
	dma_set_memory_size(DMA1, DMA_CHANNEL4, DMA_CCR_MSIZE_8BIT);
	dma_set_priority(DMA1, DMA_CHANNEL4, DMA_CCR_PL_VERY_HIGH);
	//spi_enable_rx_dma(SPI2);

        NVIC_SET_PRIORITY(NVIC_EXTI4_15_IRQ, PRIORITY_HIGHEST);
        nvic_enable_irq(NVIC_EXTI4_15_IRQ);
        //GPIO# == EXTI# (at least for # <16)
        //Setup EXTI8 (INPUT_CSN) to trigger interrupt
        exti_select_source(INPUT_CSN.pin, INPUT_CSN.port);
        exti_set_trigger(INPUT_CSN.pin, EXTI_TRIGGER_BOTH);
        exti_enable_request(INPUT_CSN.pin);
        //Setup EXTI12 (PASSTHROUGH_CSN) to trigger interrupt
        exti_select_source(PASSTHRU_CSN.pin, PASSTHRU_CSN.port);
        exti_set_trigger(PASSTHRU_CSN.pin, EXTI_TRIGGER_BOTH);
        exti_enable_request(PASSTHRU_CSN.pin);
    } else {
        spi_set_nss_high(SPI2);
        nvic_disable_irq(NVIC_EXTI4_15_IRQ);
    	spi_disable_rx_dma(SPI2);
    }
    SPI_CR2(SPI2) = 0x1700; //Force SPI into 8-bit mode
    spi_enable(SPI2);
#endif
}

void SPI_ProtoInit()
{
#if DISCOVERY
    /* This is only relavent to the Discovery board
       It uses SPI2 to control the MEMS chip, which interferest with theUniversalTx operation
       CSN is on PC.0, so pull it high to ensure the MEMS device is disabled
     */
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);
    gpio_set(GPIOC, GPIO0);
#endif

    rcc_periph_clock_enable(RCC_SPI2);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_DMA);
    PORT_mode_setup(module_enable[A7105]    , GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PORT_mode_setup(module_enable[CYRF6936] , GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PORT_mode_setup(module_enable[CC2500]   , GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PORT_mode_setup(module_enable[NRF24L01] , GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PORT_mode_setup(NRF24L01_CE             , GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);

    PORT_mode_setup(RF_MUXSEL1, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PORT_mode_setup(RF_MUXSEL2, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);

    PORT_mode_setup(INPUT_CSN , GPIO_MODE_INPUT, GPIO_PUPD_PULLUP);
    PORT_mode_setup(PASSTHRU_CSN , GPIO_MODE_INPUT, GPIO_PUPD_PULLUP);
   
    PROTOSPI_pin_set(module_enable[A7105]);
    PROTOSPI_pin_set(module_enable[CYRF6936]);
    PROTOSPI_pin_set(module_enable[CC2500]);
    PROTOSPI_pin_set(module_enable[NRF24L01]);
    PROTOSPI_pin_clear(NRF24L01_CE);
    PROTOSPI_pin_clear(RF_MUXSEL1);
    PROTOSPI_pin_clear(RF_MUXSEL2);
    Model.module = TX_MODULE_LAST;

    PORT_mode_setup(SCK,        GPIO_MODE_AF, GPIO_PUPD_NONE); 
    PORT_mode_setup(MOSI,       GPIO_MODE_AF, GPIO_PUPD_NONE); 
    PORT_mode_setup(MISO,       GPIO_MODE_AF, GPIO_PUPD_NONE); 
    gpio_set_af(GPIOB, GPIO_AF0, SCK.pin | MOSI.pin | MISO.pin);
}
