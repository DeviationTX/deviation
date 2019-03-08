#ifndef _SPIPROTO_H_
#define _SPIPROTO_H_

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

u8 PROTOSPI_read3wire();
u8 PROTOSPI_xfer(u8 byte);
#define PROTOSPI_pin_set(io) GPIO_pin_set(io)
#define PROTOSPI_pin_clear(io) GPIO_pin_clear(io)
#define PROTOSPI_pin_get(io) GPIO_pin_get(io)
#define _NOP(n) asm volatile ("nop")

#define CYRF_RESET_PIN NULL_PIN
#define AVR_RESET_PIN NULL_PIN

#endif
