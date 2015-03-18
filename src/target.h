#ifndef _TARGET_H_
#define _TARGET_H_
//#define printf if(0) printf

#define OPTIONAL 2
//Load target-specific include
#include "target_defs.h"

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

enum {
    CYRF6936,
    A7105,
    CC2500,
    NRF24L01,
#if HAS_MULTIMOD_SUPPORT
    MULTIMOD,
    MULTIMODCTL,
#endif
    TX_MODULE_LAST,
};


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

    /* Primitives */
enum DrawDir {
    DRAW_NWSE,
    DRAW_SWNE,
};
void LCD_DrawPixel(unsigned int color);
void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color);
void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir);
void LCD_DrawStop(void);

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
    LAST_PRIORITY,
};

void CLOCK_Init(void);
u32 CLOCK_getms(void);
void CLOCK_StartTimer(unsigned us, u16 (*cb)(void));
void CLOCK_StopTimer();
void CLOCK_SetMsecCallback(int cb, u32 msec);
void CLOCK_StartWatchdog();
void CLOCK_ResetWatchdog();

/*PWM/PPM functions */
void PWM_Initialize();
void PWM_Stop();
void PWM_Set(int);
void PPM_Enable(unsigned low_time, volatile u16 *pulses);

/* PPM-In functions */
#define MAX_PPM_IN_CHANNELS 8
void PPMin_TIM_Init();
void PPMin_Start();
void PPMin_Stop();


/* Sticks */
void CHAN_Init();
s16  CHAN_ReadInput(int channel);
s32  CHAN_ReadRawInput(int channel);
extern void CHAN_SetSwitchCfg(const char *str);
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
void SPI_FlashBlockWriteEnable(unsigned enable);

/* Sound */
void SOUND_Init();
void SOUND_SetFrequency(unsigned freq, unsigned volume);
void SOUND_Start(unsigned msec, u16 (*next_note_cb)());
void SOUND_StartWithoutVibrating(unsigned msec, u16(*next_note_cb)());
void SOUND_Stop();

/* Vibrating motor */
void VIBRATINGMOTOR_Init();
void VIBRATINGMOTOR_Start();
void VIBRATINGMOTOR_Stop();

/* UART & Debug */
void UART_Initialize();
void UART_Stop();

/* USB*/
void USB_Enable(unsigned type, unsigned use_interrupt);
void USB_Disable();
void USB_HandleISR();
void USB_Connect();
void HID_Enable();
void HID_Disable();

/* Filesystem */
int FS_Mount(void *FAT, const char *drive);
void FS_Unmount();
int FS_OpenDir(const char *path);
int FS_ReadDir(char *path);
void FS_CloseDir();

void _usleep(u32 usec);
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

#if HAS_MULTIMOD_SUPPORT
void SPI_SwitchStartData();
void SPI_SwitchStopData();
#endif

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
#ifdef MODULAR
  #define MODULE_CALLTYPE __attribute__((__long_call__))
#else
  #define MODULE_CALLTYPE
#endif

#endif
