#ifndef _SPIPROTO_H_
#define _SPIPROTO_H_

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

u8 PROTOSPI_read3wire();

#define PROTOSPI_pin_set(io) GPIO_pin_set(io)
#define PROTOSPI_pin_clear(io) GPIO_pin_clear(io)
#define PROTOSPI_xfer(byte) spi_xfer(PROTO_SPI.spi, byte)
#define _NOP()  asm volatile ("nop")

#endif // _SPIPROTO_H_
