#ifdef MODULAR
  #pragma long_calls
#endif

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#include "common.h"

u8 PROTOSPI_read3wire(){
    u8 data;
    while (!(SPI_SR(PROTO_SPI.spi) & SPI_SR_TXE))
        ;
    while ((SPI_SR(PROTO_SPI.spi) & SPI_SR_BSY))
        ;

    spi_disable(PROTO_SPI.spi);
    spi_set_bidirectional_receive_only_mode(PROTO_SPI.spi);
    /* Force read from SPI_DR to ensure RXNE is clear (probably not needed) */
    volatile u8 x = SPI_DR(PROTO_SPI.spi);
    (void)x;
    spi_enable(PROTO_SPI.spi);  // This starts the data read
    //Wait > 1 SPI clock (but less than 8).  clock is 4.5MHz
    asm volatile ("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
                  "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
                  "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
                  "nop\n\tnop\n\tnop\n\tnop\n\tnop");
    spi_disable(PROTO_SPI.spi);  // This ends the read window
    data = spi_read(PROTO_SPI.spi);
    spi_set_unidirectional_mode(PROTO_SPI.spi);
    spi_enable(PROTO_SPI.spi);
    return data;
}
