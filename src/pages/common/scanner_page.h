#ifndef _SCANNER_PAGE_H_
#define _SCANNER_PAGE_H_

#define MIN_RADIOCHANNEL     0x00
#define MAX_RADIOCHANNEL     0x4F

typedef void (*T_CYRF_SetTxRxMode)(int TXRX_State);
typedef void (*T_CYRF_WriteRegister)(u8 address, u8 data);
typedef u8 (*T_CYRF_ReadRegister)(u8 address);

struct scanner_page {
    u8 channelnoise[MAX_RADIOCHANNEL - MIN_RADIOCHANNEL + 1];
    u8 channel;
    u8 scanState;
    u8 time_to_scan;
    u8 enable;
    u8 scan_mode;
    u8 attenuator;

	T_CYRF_SetTxRxMode _CYRF_SetTxRxMode;
	T_CYRF_WriteRegister _CYRF_WriteRegister;
	T_CYRF_ReadRegister _CYRF_ReadRegister;

};
#endif
