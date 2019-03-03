#ifndef _SPIPROTO_H_
#define _SPIPROTO_H_

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

u8 PROTOSPI_read3wire();

#define PROTOSPI_pin_set(io) gpio_set(io.port, io.pin)
#define PROTOSPI_pin_clear(io) gpio_clear(io.port, io.pin)
#define PROTOSPI_xfer(byte) spi_xfer(SPI2, byte)
#define _NOP()  asm volatile ("nop")

#define _SPI_CYRF_RESET_PIN {GPIOB, GPIO11}
#define _SPI_AVR_RESET_PIN {GPIO_BANK_JTCK_SWCLK, GPIO_JTCK_SWCLK}

#endif // _SPIPROTO_H_
