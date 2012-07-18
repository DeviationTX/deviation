#ifndef _MODEL_H_
#define _MODEL_H_

#include "mixer.h"

/* INI file consts */
const char *MODEL_NAME;
const char *MODEL_ICON;
const char *MODEL_TYPE;

struct Model {
    char name[24];
    char icon[20];
    enum ModelType type;
    enum Protocols protocol;
    u8 num_channels;
    u16 fixed_id;
    enum TxPower tx_power;
    enum SwashType swash_type;
    enum Mode mode;
    u8 swash_invert;
    u8 Elevator_Stick;
    u8 Aileron_Stick;
    u8 Collective_Stick;
    struct Trim trims[NUM_TRIMS];
    struct Mixer mixers[NUM_MIXERS];
    struct Limit limits[NUM_CHANNELS];
    u8 template[NUM_CHANNELS];
};
extern struct Model Model;
extern const char * const RADIO_TX_POWER_VAL[];
extern const char * const RADIO_PROTOCOL_VAL[];

u8 CONFIG_ReadModel(u8 model_num);
u8 CONFIG_WriteModel(u8 model_num);
u8 CONFIG_SaveModelIfNeeded();
u8 CONFIG_GetCurrentModel();
const char *CONFIG_GetIcon(enum ModelType type);
const char *CONFIG_GetCurrentIcon();
enum ModelType CONFIG_ParseModelType(const char *value);
void CONFIG_ParseModelName(char *name, const char *value);
#endif /*_MODEL_H_*/
