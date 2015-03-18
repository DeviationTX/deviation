#ifndef _SPIPROTO_H_
#define _SPIPROTO_H_

u8 PROTOSPI_read3wire();
u8 PROTOSPI_xfer(u8 byte);
#define PROTOSPI_pin_set(io) if(0) {}
#define PROTOSPI_pin_clear(io) if(0) {}
#define _NOP() if(0) {}

#define SPI_SwitchStartData() if(0) {}
#define SPI_SwitchStopData() if(0) {}

static const struct mcu_pin CYRF_RESET_PIN ={0, 0};
static const struct mcu_pin AVR_RESET_PIN ={0,0};
#pragma weak A7105_Reset
#pragma weak CC2500_Reset
#pragma weak CYRF_Reset
#endif // _SPIPROTO_H_
