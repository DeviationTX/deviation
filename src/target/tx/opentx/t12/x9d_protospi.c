
#include "common.h"
#include "protocol/interface.h"

int SPI_ProtoGetPinConfig(int module, int state) {(void)module; (void)state; return 0;}
u8 PROTOSPI_read3wire() { return 0x00; }
u8 PROTOSPI_xfer(u8 byte) { return byte; }
void SPI_ProtoInit() {}
int MCU_SetPin(struct mcu_pin *port, const char *name) {return 0;}
void MCU_InitModules() {}
int SPI_ConfigSwitch(unsigned csn_high, unsigned csn_low) {
    (void)csn_high;
    (void)csn_low;
    return 0;
}
