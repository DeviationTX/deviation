/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Deviation is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "common.h"
#include "mixer.h"
#include "config/tx.h"
#include "emu.h"

void start_event_loop()
{
}


struct touch SPITouch_GetCoords()
{
    struct touch t = {256, 16, 0, 0};
    return t;
}

int SPITouch_IRQ()
{
    return 0;
}

u32 ScanButtons()
{
    return 1;
}

int PWR_CheckPowerSwitch()
{
    return 0;
}

void PWR_Shutdown()
{
}

u32 ReadFlashID()
{
    return 0;
}

void SPITouch_Calibrate(s32 xscale, s32 yscale, s32 xoff, s32 yoff)
{
    (void)xscale;
    (void)yscale;
    (void)xoff;
    (void)yoff;
}

void CLOCK_Init()
{
}
void CLOCK_StartTimer(unsigned us, u16 (*cb)(void))
{
    (void)us;
    (void)cb;
}

void CLOCK_StopTimer()
{
}

void CLOCK_SetMsecCallback(int cb, u32 msec)
{
    (void)msec;
    (void)cb;
}

void CLOCK_ClearMsecCallback(int cb)
{
    (void)cb;
}

u32 CLOCK_getms()
{
    return 100000;
}

void PWR_Sleep()
{
}

void _usleep(u32 usec) {
    usleep(usec);
}
void TxName(u8 *var, int len) {
    const u8 model[] = "EMU_STRING";
    if(len > 12)
         len = 12;
    memcpy(var, model, len - 1);
    var[len-1] = 0;
}
void MSC_Enable() {}
void MSC_Disable() {}
void HID_Enable() {}
void HID_Disable() {}
void HID_Write(s8 *pkt, u8 size) {
    (void)pkt;
    (void)size;
}
void Initialize_ButtonMatrix() {}
void PWR_Init(void) {}
unsigned  PWR_ReadVoltage() { return (DEFAULT_BATTERY_ALARM + 1000); }
void ADC_Init() {}
void SWITCH_Init() {}

void CLOCK_StartWatchdog() {}
void CLOCK_ResetWatchdog() {}
void CLOCK_RunMixer() {}
void CLOCK_StartMixer() {}
volatile mixsync_t mixer_sync;

u32  SPIFlash_ReadID() { return 0x12345678; }
void SPIFlash_BlockWriteEnable(unsigned enable) {(void)enable;}
void SPITouch_Init() {}

u8 *BOOTLOADER_Read(int idx) {
    static u8 str[3][80] = {
        "",
        EMU_STRING,
        };
    u8 ret = 0;
    switch(idx) {
        case BL_ID: ret = 1; break;
    }
    return str[ret];
}

void UART_Initialize() {}
u8 UART_Send(u8 *data, u16 len) { (void)data; (void)len; return 0;}
void UART_Stop() {}
void UART_SetDataRate(u32 bps) { (void)bps;}
void UART_SetFormat(int bits, uart_parity parity, uart_stopbits stopbits) {(void) bits; (void) parity; (void) stopbits;}
void UART_StartReceive(usart_callback_t isr_callback) {(void) isr_callback;}
void UART_StopReceive() {}
void UART_SetDuplex(uart_duplex duplex) {(void) duplex;}

void init_err_handler() {}

void BACKLIGHT_Init() {}
void BACKLIGHT_Brightness(unsigned brightness) { printf("Backlight: %d\n", brightness); }
void LCD_Contrast(unsigned contrast) { printf("Contrast: %d\n", contrast); }

void VIBRATINGMOTOR_Init() {}
void VIBRATINGMOTOR_Start() {}
void VIBRATINGMOTOR() {}

void VIDEO_SetChannel(int ch) {printf("Video Channel: %d\n", ch); }
void VIDEO_Enable(int on)     {printf("Video Enable: %s\n", on ? "On" : "Off"); }
void VIDEO_Brightness(int brightness) { printf("Video Brightness: %d\n", brightness); }
void VIDEO_Contrast(int contrast) { printf("Video Contrast: %d\n", contrast); }
u8 VIDEO_GetStandard() { return 0xFE; }
void VIDEO_SetStandard(u8 standard) { printf("Video Standard: %d\n", standard); }

void PPMin_Start() {}
void PPMin_Stop() {}
void PPMin_TIM_Init() {}
volatile u8 ppmSync;
volatile s32 ppmChannels[MAX_PPM_IN_CHANNELS];
volatile u8 ppmin_num_channels;

void SSER_StartReceive(sser_callback_t isr_callback) { (void)isr_callback;}
void SSER_Initialize() {}
void SSER_Stop() {}

void PXX_Enable(u8 *packet) { (void)packet; }

void MCU_SerialNumber(u8 *var, int len)
{
    int l = len > 12 ? 12 : len;
    if(Transmitter.txid) {
        u32 id[4];
        u32 seed = 0x4d3ab5d0ul;
        for(int i = 0; i < 4; i++)
            rand32_r(&seed, Transmitter.txid >> (8*i));
        for(int i = 0; i < 4; i++)
            id[i] = rand32_r(&seed, 0x00);
        memcpy(var, &id[1], len);
        return;
    }
    const char data[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                         0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    memcpy(var, data, l);
}
void PWR_JumpToProgrammer() {}

void LED_Init() {}
