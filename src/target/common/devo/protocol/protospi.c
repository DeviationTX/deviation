#ifdef MODULAR
  #pragma long_calls
#endif

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#include "common.h"

u8 PROTOSPI_read3wire(){
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
