// FIXME: We probably can't use the UART at all

#include "common.h"
void UART_SetDataRate(u32 bps) { (void)bps; }
void UART_SetFormat(int bits, uart_parity parity, uart_stopbits stopbits) {
    (void)bits;
    (void)parity;
    (void)stopbits;
}
u8 UART_Send(u8 *data, u16 len) {
    (void)data;
    (void)len;
    return 0;
}

void UART_StartReceive(usart_callback_t *isr_callback) {
    (void)isr_callback;
}

void UART_StopReceive() {}
void UART_SetDuplex(uart_duplex duplex) { (void)duplex; }


