#ifndef _SPIPROTO_H_
#define _SPIPROTO_H_

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "ports.h"

u8 PROTOSPI_read3wire();
uint8_t spi_xfer8(uint32_t spi, uint8_t data);

#define spi_xfer             DO_NOT_USE
#define PROTOSPI_pin_set     PORT_pin_set
#define PROTOSPI_pin_clear   PORT_pin_clear
#define PROTOSPI_xfer(byte)  spi_xfer8(SPI2, byte)

#define _NOP()  asm volatile ("nop")

#define MODULE_ENABLE module_enable
#endif // _SPIPROTO_H_
