#ifndef _SPIPROTO_H_
#define _SPIPROTO_H_

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

inline u8 PROTOSPI_read3wire(){
    u8 data;
    while(!(SPI_SR(SPI2) & SPI_SR_TXE))
        ;
    while((SPI_SR(SPI2) & SPI_SR_BSY))
        ;

    spi_disable(SPI2);
    spi_set_bidirectional_receive_only_mode(SPI2);
    /* Force read from SPI_DR to ensure RXNE is clear (probably not needed) */
    volatile u8 x = SPI_DR(SPI2);
    (void)x;
    spi_enable(SPI2);  //This starts the data read
    //Wait > 1 SPI clock (but less than 8).  clock is 4.5MHz
    asm volatile ("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
                  "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
                  "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
                  "nop\n\tnop\n\tnop\n\tnop\n\tnop");
    spi_disable(SPI2); //This ends the read window
    data = spi_read(SPI2);
    spi_set_unidirectional_mode(SPI2);
    spi_enable(SPI2);
    return data;
}

#define PROTOSPI_pin_set(io) gpio_set(io.port,io.pin)
#define PROTOSPI_pin_clear(io) gpio_clear(io.port,io.pin)
#define PROTOSPI_xfer(byte) spi_xfer(SPI2, byte)
#define _NOP()  asm volatile ("nop")

static const struct mcu_pin CYRF_RESET_PIN ={GPIOB, GPIO11};
static const struct mcu_pin AVR_RESET_PIN ={GPIO_BANK_JTCK_SWCLK, GPIO_JTCK_SWCLK};

#endif // _SPIPROTO_H_
