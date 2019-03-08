#ifndef _TARGET_H_
#define _TARGET_H_
//#define printf if(0) printf

#define OPTIONAL 2

//These are used to enable(1) or disable(2) the internal filesystem code */
#define USE_NATIVE_FS   0
#define USE_INTERNAL_FS 1

//Load target-specific include
#include "target_defs.h"
#include "target_all.h"

/* List of channels provided by the Tx */
#define CHANDEF(x) INP_##x,
#define UNDEF_INP
enum {
    INP_NONE,
    #include "capabilities.h"
    INP_LAST,
};
#undef CHANDEF
#undef UNDEF_INP
/* List of buttons provided by the Tx */
#define BUTTONDEF(x) BUT_##x,
enum {
    BUT_NONE,
    #include "capabilities.h"
    BUT_LAST,
};
#undef BUTTONDEF

#define CHAN_ButtonMask(btn) (btn ? (1 << (btn - 1)) : 0)

#define NUM_TX_INPUTS (INP_LAST - 1)
#define NUM_INPUTS (NUM_TX_INPUTS)
#define NUM_TX_BUTTONS (BUT_LAST - 1)

enum Radio {
    CYRF6936,
    A7105,
    CC2500,
    NRF24L01,
    MULTIMOD,
    R9M,
    TX_MODULE_LAST,
};

#ifdef USE_PBM_IMAGE
    #define IMG_EXT  ".pbm"
#else
    #define IMG_EXT  ".bmp"
#endif

#define SWITCH_ADDRESS 0xFFFFFFFF
/* The following functions must be provided by every target */

/* General Functions */
void TxName(u8 *var, int len);

/* Backlight Functions */
void BACKLIGHT_Init();
void BACKLIGHT_Brightness(unsigned brightness);

/* Display Functions */
void LCD_Init();
void LCD_Contrast(unsigned contrast);
void LCD_ForceUpdate();
void LCD_CreateMappedWindow(unsigned val, unsigned x, unsigned y, unsigned w, unsigned h);
void LCD_SetMappedWindow(unsigned val);
void LCD_UnmapWindow(unsigned val);
unsigned LCD_GetMappedWindow();
void LCD_LoadFont(int idx, const char *file, int x_off, int y_off, int w, int h);

    /* Primitives */
enum DrawDir {
    DRAW_NWSE,
    DRAW_SWNE,
};
void LCD_DrawPixel(unsigned int color);
void LCD_DrawMappedPixel(unsigned int color);
void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color);
void LCD_DrawMappedPixelXY(unsigned int x, unsigned int y, unsigned int color);
void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir);
void LCD_DrawMappedStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir);
void LCD_DrawStop(void);
void LCD_DrawMappedStop(void);
void LCD_ShowVideo(u8 enable);

void VIDEO_SetChannel(int ch);
void VIDEO_Enable(int on);
void VIDEO_Contrast(int contrast);
void VIDEO_Brightness(int brightness);
u8 VIDEO_GetStandard(void);
void VIDEO_SetStandard(u8 standard);

/* Touchscreen */
struct touch {
    u16 x;
    u16 y;
    u16 z1;
    u16 z2;
};
void SPITouch_Init();
struct touch SPITouch_GetCoords();
int SPITouch_IRQ();
void SPITouch_Calibrate(s32 xscale, s32 yscale, s32 xoff, s32 yoff);

/* Buttons and switches */
void Initialize_ButtonMatrix();
u32 ScanButtons();

/* Rotary */
void ROTARY_Init();
u32 ROTARY_Scan();

/* Power functions */
void PWR_Init(void);
unsigned  PWR_ReadVoltage(void);
int  PWR_CheckPowerSwitch();
void PWR_Shutdown();
void PWR_Sleep();

/* Clock functions */
#define LOW_PRIORITY_MSEC 100
#define MEDIUM_PRIORITY_MSEC   5
enum MsecCallback {
    MEDIUM_PRIORITY,
    LOW_PRIORITY,
    TARGET_PRIORITY,
    NUM_MSEC_CALLBACKS,
};

void CLOCK_Init(void);
u32 CLOCK_getms(void);
void CLOCK_StartTimer(unsigned us, u16 (*cb)(void));
void CLOCK_StopTimer();
void CLOCK_SetMsecCallback(int cb, u32 msec);
void CLOCK_ClearMsecCallback(int cb);
void CLOCK_StartWatchdog();
void CLOCK_ResetWatchdog();
void CLOCK_RunMixer();
void CLOCK_StartMixer();
typedef enum {
    MIX_TIMER,
    MIX_NOT_DONE,
    MIX_DONE
} mixsync_t;
extern volatile mixsync_t mixer_sync;

/*PWM/PPM functions */
#define PPM_POLARITY_NORMAL 0
#define PPM_POLARITY_INVERTED 1
void PWM_Initialize();
void PWM_Stop();
void PWM_Set(int);
void PPM_Enable(unsigned active_time, volatile u16 *pulses, u8 num_pulses, u8 polarity);
void PXX_Enable(u8 *packet);

/* PPM-In functions */
#define MAX_PPM_IN_CHANNELS 8
void PPMin_TIM_Init();
void PPMin_Start();
void PPMin_Stop();


/* Sticks */
void CHAN_Init();
s32  CHAN_ReadInput(int channel);
s32  CHAN_ReadRawInput(int channel);
extern void CHAN_SetSwitchCfg(const char *str);
extern void CHAN_SetButtonCfg(const char *str);
#define CHAN_ButtonIsPressed(buttons, btn) (buttons & (CHAN_ButtonMask(btn)))

/* SPI Flash */
void SPIFlash_Init();
u32  SPIFlash_ReadID();
void SPIFlash_EraseSector(u32 sectorAddress);
void SPIFlash_BulkErase();
void SPIFlash_WriteBytes(u32 writeAddress, u32 length, const u8 * buffer);
void SPIFlash_WriteByte(u32 writeAddress, const unsigned byte);
void SPIFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer);
int  SPIFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer);
void SPIFlash_BlockWriteEnable(unsigned enable);

void MCUFlash_Init();
u32  MCUFlash_ReadID();
void MCUFlash_EraseSector(u32 sectorAddress);
void MCUFlash_BulkErase();
void MCUFlash_WriteBytes(u32 writeAddress, u32 length, const u8 * buffer);
void MCUFlash_WriteByte(u32 writeAddress, const unsigned byte);
void MCUFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer);
int  MCUFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer);
void MCUFlash_BlockWriteEnable(unsigned enable);

void PARFlash_Init();
void PARFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer);
int  PARFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer);

void MMC_Init();

#define FLASHTYPE_SPI 1
#define FLASHTYPE_MCU 2
#define FLASHTYPE_MMC 3

#if FLASHTYPE == FLASHTYPE_SPI
    #define STORAGE_Init()   SPIFlash_Init()
    #define STORAGE_ReadID() SPIFlash_ReadID()
    #define STORAGE_WriteEnable(enable) SPIFlash_BlockWriteEnable(enable)
    #define STORAGE_ReadBytes SPIFlash_ReadBytes
    #define STORAGE_ReadBytesStopCR SPIFlash_ReadBytesStopCR
    #define STORAGE_WriteBytes SPIFlash_WriteBytes
    #define STORAGE_EraseSector SPIFlash_EraseSector
#elif FLASHTYPE == FLASHTYPE_MCU
    #define STORAGE_Init()   MCUFlash_Init()
    #define STORAGE_ReadID() MCUFlash_ReadID()
    #define STORAGE_WriteEnable(enable) MCUFlash_BlockWriteEnable(enable)
    #define STORAGE_ReadBytes MCUFlash_ReadBytes
    #define STORAGE_ReadBytesStopCR MCUFlash_ReadBytesStopCR
    #define STORAGE_WriteBytes MCUFlash_WriteBytes
    #define STORAGE_EraseSector MCUFlash_EraseSector
#elif FLASHTYPE == FLASHTYPE_MMC
    #define STORAGE_WriteEnable(enable) do {} while (0)
    #define STORAGE_Init() MMC_Init()
#else
#error Define FLASHTYPE to FLASHTYPE_MCU or FLASHTYPE_SPI or FLASHTYPE_MMC
#endif


/* Sound */
void SOUND_Init();
void SOUND_SetFrequency(unsigned freq, unsigned volume);
void SOUND_Start(unsigned msec, u16 (*next_note_cb)(), u8 vibrate);
void SOUND_StartWithoutVibrating(unsigned msec, u16(*next_note_cb)());
void SOUND_Stop();

/* Vibrating motor */
void VIBRATINGMOTOR_Init();
void VIBRATINGMOTOR_Start();
void VIBRATINGMOTOR_Stop();

/* LED Driver */
void LED_Init();
void LED_Status(u8 on);
void LED_RF(u8 on);
void LED_Storage(u8 on);

/* UART & Debug */
typedef enum {
    UART_STOPBITS_1,
    UART_STOPBITS_1_5,
    UART_STOPBITS_2,
} uart_stopbits;
typedef enum {
    UART_PARITY_EVEN,
    UART_PARITY_ODD,
    UART_PARITY_NONE,
} uart_parity;
typedef enum {
    UART_DUPLEX_FULL,
    UART_DUPLEX_HALF,
} uart_duplex;
void UART_Initialize();
void UART_Stop();
u8 UART_Send(u8 *data, u16 len);
void UART_SetDataRate(u32 bps);
void UART_SetFormat(int bits, uart_parity parity, uart_stopbits stopbits);
typedef void usart_callback_t(u8 ch, u8 status);
/* callback status byte bit fields */
#define UART_RX_RXNE (1 << 5)  //USART_SR_RXNE - rx buffer not empty
#define UART_SR_ORE  (1 << 3)  //USART_SR_ORE  - rx buffer overrun error
#define UART_SR_NE   (1 << 2)  //USART_SR_NE   - noise error
#define UART_SR_FE   (1 << 1)  //USART_SR_FE   - framing error
#define UART_SR_PE   (1 << 0)  //USART_SR_PE   - parity error
void UART_StartReceive(usart_callback_t isr_callback);
void UART_StopReceive();
void UART_SetDuplex(uart_duplex duplex);
void UART_SendByte(u8 x);
typedef void sser_callback_t(u8 data);
void SSER_StartReceive(sser_callback_t isr_callback);
void SSER_Initialize();
void SSER_Stop();

/* USB*/
void USB_Enable(unsigned use_interrupt);
void USB_Disable();
void USB_HandleISR();
void USB_Connect();

void HID_Enable();
void HID_Disable();

void MSC_Enable();
void MSC_Disable();

/* Filesystem */
int FS_Init();
int FS_Mount(void *FAT, const char *drive);
void FS_Unmount();
int FS_OpenDir(const char *path);
int FS_ReadDir(char *path);
void FS_CloseDir();

void _usleep(u32 usec);
void _msleep(u32 msec);
#define usleep _usleep

/* Abstract bootloader access */
enum {
    BL_ID = 0,
};
u8 *BOOTLOADER_Read(int idx);

#define PROTO_HAS_CYRF6936
#define PROTO_HAS_A7105
#define PROTO_HAS_CC2500
#define PROTO_HAS_NRF24L01
//Ensure functions are loaded for protocol modules
void SPI_ProtoInit();
void SPI_AVRProgramInit();
int SPI_ConfigSwitch(unsigned csn_high, unsigned csn_low);
int SPI_ProtoGetPinConfig(int module, int state);
u32 AVR_StartProgram();
int AVR_Erase();
int AVR_Program(u32 address, u8 *data, int pagesize);
int AVR_SetFuses();
int AVR_ResetFuses();
int AVR_VerifyFuses();
int AVR_Verify(u8 *data, int size);

struct mcu_pin;
void MCU_InitModules();
int MCU_SetPin(struct mcu_pin *, const char *name);
const char *MCU_GetPinName(char *str, struct mcu_pin *);
void MCU_SerialNumber(u8 *var, int len);

#if defined HAS_4IN1_FLASH && HAS_4IN1_FLASH
void SPISwitch_Init();
unsigned SPISwitch_Present();
unsigned SPISwitch_FlashPresent();
void SPISwitch_CS_HI(int module);
void SPISwitch_CS_LO(int module);
void SPISwitch_UseFlashModule();
void SPISwitch_CYRF6936_RESET(int state);
void SPISwitch_NRF24L01_CE(int state);
#else
#define SPISwitch_Init()
#define SPISwitch_Present() 0
#define SPISwitch_FlashPresent() 0
#define SPISwitch_CS_HI(module)
#define SPISwitch_CS_LO(module)
#define SPISwitch_UseFlashModule()
#define SPISwitch_CYRF6936_RESET(state)
#define SPISwitch_NRF24L01_CE(state)
#endif

#endif
