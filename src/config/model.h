#ifndef _MODEL_H_
#define _MODEL_H_

#include "mixer.h"
#include "timer.h"
#include "pagecfg.h"

/* INI file consts */
const char *MODEL_NAME;
const char *MODEL_ICON;
const char *MODEL_TYPE;
const char *MODEL_TEMPLATE;

struct Model {
    char name[24];
    char icon[20];
    enum ModelType type;
    enum Protocols protocol;
    u8 num_channels;
    u32 fixed_id;
    enum TxPower tx_power;
    enum SwashType swash_type;
    u8 swash_invert;
    u8 collective_source;
    struct Trim trims[NUM_TRIMS];
    struct Mixer mixers[NUM_MIXERS];
    struct Limit limits[NUM_OUT_CHANNELS];
    struct Timer timer[NUM_TIMERS];
    u8 template[NUM_CHANNELS];
    struct PageCfg pagecfg;
    u8 safety[NUM_SOURCES];
};
extern struct Model Model;
extern const char * const RADIO_TX_POWER_VAL[TXPOWER_LAST];

u8 CONFIG_ReadModel(u8 model_num);
u8 CONFIG_WriteModel(u8 model_num);
u8 CONFIG_SaveModelIfNeeded();
u8 CONFIG_GetCurrentModel();
const char *CONFIG_GetIcon(enum ModelType type);
const char *CONFIG_GetCurrentIcon();
enum ModelType CONFIG_ParseModelType(const char *value);
void CONFIG_ParseModelName(char *name, const char *value);
void CONFIG_ResetModel();
u8 CONFIG_ReadTemplate(u8 template_num);
#endif /*_MODEL_H_*/
