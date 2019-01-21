#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

//Magic macro to check enum size
//#define ctassert(n,e) extern unsigned char n[(e)?0:-1]
#define ctassert(COND,MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#define _UNUSED   __attribute__ ((unused))
#define NO_INLINE __attribute__ ((noinline))

#define TEMPSTRINGLENGTH 400 //This is the max dialog size (80 characters * 5 lines)
                             //We could reduce this to ~240 on the 128x64 screens
                             //But only after all sprintf are replaced with snprintf
                             //Maybe move this to target_defs.h
extern char tempstring[TEMPSTRINGLENGTH];


typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#include "target.h"
#include "std.h"

//Compatibility with Atmega
#define FLASHBYTETABLE static const u8
#define FLASHWORDTABLE static const u16
#define pgm_read_word(x) (*(x))
#define pgm_read_byte(x) (*(x))
void PROTO_CS_HI(int module);
void PROTO_CS_LO(int module);

extern volatile s32 Channels[NUM_OUT_CHANNELS];
extern const char DeviationVersion[33];

/* Temproary definition until we have real translation */
#define _tr_noop(x) x
#if !SUPPORT_MULTI_LANGUAGE
#define _tr(x) x
#else
const char *_tr(const char *str);
#endif

void CONFIG_ReadLang(u8 idx);
void CONFIG_EnableLanguage(int state);
int CONFIG_IniParse(const char* filename,
         int (*handler)(void*, const char*, const char*, const char*),
         void* user);
u8 CONFIG_IsModelChanged();
u8 CONFIG_SaveModelIfNeeded();
void CONFIG_SaveTxIfNeeded();
extern const char * const MODULE_NAME[TX_MODULE_LAST];

/* LCD primitive functions */
void LCD_Clear(unsigned int color);
    /* Strings */
void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c);
void LCD_PrintChar(u32 c);
void LCD_PrintStringXY(unsigned int x, unsigned int y, const char *str);
void LCD_PrintString(const char *str);
void LCD_SetXY(unsigned int x, unsigned int y);
void LCD_GetStringDimensions(const u8 *str, u16 *width, u16 *height);
void LCD_GetCharDimensions(u32 c, u16 *width, u16 *height);
u8 LCD_SetFont(unsigned int idx);
u8  LCD_GetFont();
void LCD_SetFontColor(u16 color);
    /* Graphics */
void LCD_DrawCircle(u16 x0, u16 y0, u16 r, u16 color);
void LCD_FillCircle(u16 x0, u16 y0, u16 r, u16 color);
void LCD_DrawLine(u16 x0, u16 y0, u16 x1, u16 y1, u16 color);
void LCD_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void LCD_DrawFastHLine(u16 x, u16 y, u16 w, u16 color);
void LCD_DrawDashedVLine(int16_t x, int16_t y, int16_t h, int16_t space, uint16_t color);
void LCD_DrawDashedHLine(int16_t x, int16_t y, int16_t w, int16_t space, uint16_t color);
void LCD_DrawRect(u16 x, u16 y, u16 w, u16 h, u16 color);
void LCD_FillRect(u16 x, u16 y, u16 w, u16 h, u16 color);
void LCD_DrawRoundRect(u16 x, u16 y, u16 w, u16 h, u16 r, u16 color);
void LCD_FillRoundRect(u16 x, u16 y, u16 w, u16 h, u16 r, u16 color);
void LCD_DrawTriangle(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void LCD_FillTriangle(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void LCD_DrawWindowedImageFromFile(u16 x, u16 y, const char *file, s16 w, s16 h, u16 x_off, u16 y_off);
void LCD_DrawImageFromFile(u16 x, u16 y, const char *file);
u8 LCD_ImageIsTransparent(const char *file);
u8 LCD_ImageDimensions(const char *file, u16 *w, u16 *h);
void LCD_DrawUSBLogo(int lcd_width, int lcd_height);

/* Music */

/* Mixer functions */
void MIXER_CalcChannels();

/* GUI Pages */
void PAGE_Init();
void PAGE_Change(int dir);
void PAGE_Event();
int PAGE_DialogVisible();
void PAGE_ShowSafetyDialog();
void PAGE_CloseBindingDialog();
void PAGE_ShowBindingDialog(u8 update);
void PAGE_ShowLowBattDialog();
void PAGE_ShowResetPermTimerDialog(void *guiObject, void *data);
void PAGE_ShowInvalidModule();
void PAGE_ShowModuleDialog(const char **missing);
void PAGE_ShowWarning(const char *title, const char *str);
void PAGE_ShowTelemetryAlarm();
const char *PAGE_GetName(int idx);
int PAGE_GetNumPages();
int PAGE_GetStartPage();
int PAGE_GetID();
int PAGE_ModelDoneEditing();

/* Protocol */
#define PROTODEF(proto, module, map, init, name) proto,
enum Protocols {
    PROTOCOL_NONE = 0,
    #include "protocol/protocol.h"
    PROTOCOL_COUNT,
};
#undef PROTODEF
extern const u8 *CurrentProtocolChannelMap;
#define PROTO_MAP_LEN 5

enum ModelType {
    MODELTYPE_HELI,
    MODELTYPE_PLANE,
    MODELTYPE_MULTI,
};
#define MODELTYPE_LAST (MODELTYPE_MULTI + 1)

enum TxPower {
    TXPOWER_100uW,
    TXPOWER_300uW,
    TXPOWER_1mW,
    TXPOWER_3mW,
    TXPOWER_10mW,
    TXPOWER_30mW,
    TXPOWER_100mW,
    TXPOWER_150mW,
    TXPOWER_LAST,
};

void PROTOCOL_Init(u8 force);
void PROTOCOL_Load(int no_dlg);
void PROTOCOL_DeInit();
u8 PROTOCOL_WaitingForSafe();
u64 PROTOCOL_CheckSafe();
u32 PROTOCOL_Binding();
u8 PROTOCOL_AutoBindEnabled();
void PROTOCOL_Bind();
void PROTOCOL_SetBindState(u32 msec);
int PROTOCOL_NumChannels();
u8 PROTOCOL_GetTelemCapability();
int PROTOCOL_DefaultNumChannels();
void PROTOCOL_CheckDialogs();
u32 PROTOCOL_CurrentID();
const char * PROTOCOL_Name();
const char * PROTOCOL_GetName(u16 idx);
const char **PROTOCOL_GetOptions();
const u8* PROTOCOL_GetChannelMap();
void PROTOCOL_SetOptions();
int PROTOCOL_GetTelemetryState();
int PROTOCOL_GetTelemetryType();
int PROTOCOL_MapChannel(int input, int default_ch);
int PROTOCOL_HasModule(int idx);
int PROTOCOL_HasPowerAmp(int idx);
int PROTOCOL_SetSwitch(int module);
int PROTOCOL_SticksMoved(int init);
void PROTOCOL_InitModules();
void PROTOCOL_ResetTelemetry();
enum Radio PROTOCOL_GetRadio(u16 idx);


/* Input */
const char *INPUT_SourceName(char *str, unsigned src);
const char *INPUT_SourceNameReal(char *str, unsigned src);
const char *INPUT_SourceNameAbbrevSwitch(char *str, unsigned src);
const char *INPUT_SourceNameAbbrevSwitchReal(char *str, unsigned src);
int INPUT_GetAbbrevSource(int origval, int newval, int dir);
int INPUT_SwitchPos(unsigned src);
int INPUT_NumSwitchPos(unsigned src);
int INPUT_GetFirstSwitch(int src);
int INPUT_SelectSource(int src, int dir, u8 *changed);
int INPUT_SelectInput(int src, int newsrc, u8 *changed);
int INPUT_SelectAbbrevSource(int src, int dir);

const char *INPUT_MapSourceName(unsigned idx, unsigned *val);
const char *INPUT_ButtonName(unsigned src);
void INPUT_CheckChanges(void);

/* Misc */
void Delay(u32 count);
u32 Crc(const void *buffer, u32 size);
const char *utf8_to_u32(const char *str, u32 *ch);
int exact_atoi(const char *str); //Like atoi but will not decode a number followed by non-number
size_t strlcpy(char* dst, const char* src, size_t bufsize);
void tempstring_cpy(const char* src);
int fexists(const char *file);
u32 rand32_r(u32 *seed, u8 update); //LFSR based PRNG
u32 rand32(); //LFSR based PRNG
extern volatile u8 priority_ready;
void debug_timing(u32 type, int startend); //This is only defined if TIMING_DEBUG is defined
void DEBUGLOG_Putc(char c);
/* Battery */
#define BATTERY_CRITICAL 0x01
#define BATTERY_LOW      0x02
u8 BATTERY_Check();

/* Mixer mode */
typedef enum {
    MIXER_ADVANCED = 0x01,
    MIXER_STANDARD = 0x02,
} MixerMode;
void PAGE_ShowInvalidStandardMixerDialog(void *guiObj);
void STDMIXER_Preset();
void STDMIXER_SetChannelOrderByProtocol();
unsigned STDMIXER_ValidateTraditionModel();
const char *STDMIXER_ModeName(int mode);
void STDMIXER_InitSwitches();
void STDMIXER_SaveSwitches();
const char *GetElemName(int type);
const char *GetBoxSource(char *str, int src);
const char *GetBoxSourceReal(char *str, int src);
void RemapChannelsForProtocol(const u8 *oldmap);
#define PPMin_Mode() (Model.num_ppmin >> 6)
#endif
