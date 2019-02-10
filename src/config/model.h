#define NUM_TRIM_ELEMS 6
#define NUM_BOX_ELEMS 8
#define NUM_BAR_ELEMS 8
#define NUM_TOGGLE_ELEMS 4

#ifndef _MODEL_H_
#define _MODEL_H_

#include "mixer.h"
#include "timer.h"
#include "rtc.h"
#include "telemetry.h"
#include "datalog.h"
#include "pagecfg.h"
#if HAS_EXTENDED_AUDIO
#include "music.h"
#endif

/* INI file consts */
extern const char MODEL_NAME[];
extern const char MODEL_ICON[];
extern const char MODEL_TYPE[];
extern const char MODEL_TEMPLATE[];
#define UNKNOWN_ICON ("media/noicon" IMG_EXT)

//This cannot be computed, and must be manually updated
#define NUM_PROTO_OPTS 8
#define VIRT_NAME_LEN 10

struct Model {
    u32 fixed_id;
    enum ModelType type;
    enum Protocols protocol;
    s16 proto_opts[NUM_PROTO_OPTS];
    u8 num_channels;
    u8 num_ppmin;
    u16 ppmin_centerpw;
    u16 ppmin_deltapw;
    u8 train_sw;
    enum Radio radio;
    enum TxPower tx_power;
    enum SwashType swash_type;
    u8 swash_invert;
    u8 swashmix[3];
    char name[24];
    char icon[24];
    char virtname[NUM_VIRT_CHANNELS][VIRT_NAME_LEN];
    u8 templates[NUM_CHANNELS];
    u8 safety[NUM_SOURCES+1];
    MixerMode mixer_mode;
    s8 ppm_map[MAX_PPM_IN_CHANNELS];
    u8 padding_1[2];
#if HAS_PERMANENT_TIMER
    u32 permanent_timer;
#endif
#if HAS_VIDEO
    u8 videosrc;
    u8 videoch;
    s8 video_contrast;
    s8 video_brightness;
#endif
    struct TelemetryAlarm alarms[TELEM_NUM_ALARMS];
    struct Trim trims[NUM_TRIMS];
    struct Mixer mixers[NUM_MIXERS];
    struct Limit limits[NUM_OUT_CHANNELS];
    struct Timer timer[NUM_TIMERS];
    struct PageCfg2 pagecfg2;
#if HAS_DATALOG
    struct datalog datalog;
#endif
#if HAS_EXTENDED_AUDIO
    struct Voice voice;
#endif // HAS_EXTENDED_AUDIO
#if HAS_EXTENDED_TELEMETRY
    s32 ground_level;
#endif
};
extern struct Model Model;
const char * radio_tx_power_val(enum Radio, enum TxPower);
extern const u8 RADIO_TX_POWER_COUNT[TX_MODULE_LAST];

u8 CONFIG_ReadModel(u8 model_num);
u8 CONFIG_WriteModel(u8 model_num);
u8 CONFIG_GetCurrentModel();
const char *CONFIG_GetIcon(enum ModelType type);
const char *CONFIG_GetCurrentIcon();
enum ModelType CONFIG_ParseModelType(const char *value);
void CONFIG_ParseIconName(char *name, const char *value);
void CONFIG_ResetModel();
u8 CONFIG_ReadTemplateByIndex(u8 template_num);
u8 CONFIG_ReadTemplate(const char *filename);
u8 CONFIG_ReadLayout(const char *filename);

#endif /*_MODEL_H_*/
