#ifndef _SPIPROTO_H_
#define _SPIPROTO_H_

#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/spi.h>

u8 PROTOSPI_read3wire();
u8 PROTOSPI_xfer(u8 byte);
#define PROTOSPI_pin_set(io) gpio_set((io).port, (io).pin)
#define PROTOSPI_pin_clear(io) gpio_clear((io).port, (io).pin)
#define _NOP(n) asm volatile ("nop")

#define _SPI_CYRF_RESET_PIN {0, 0}
#define _SPI_AVR_RESET_PIN {0, 0}

#endif
