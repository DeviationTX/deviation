#ifndef _MUSIC_H_
#define _MUSIC_H_

#include <telemetry.h>
#include <timer.h>

enum Music {
    MUSIC_STARTUP = 0,
    MUSIC_SHUTDOWN,
    MUSIC_VOLUME,
    MUSIC_ALARM1,
    MUSIC_ALARM2,
    MUSIC_ALARM3,
    MUSIC_ALARM4,
    MUSIC_BATT_ALARM,
    MUSIC_DONE_BINDING,
    MUSIC_TIMER_WARNING,
    MUSIC_KEY_PRESSING,
    MUSIC_SAVING,
    MUSIC_MAXLEN,
    MUSIC_TELEMALARM1,
    MUSIC_TELEMALARM2,
    MUSIC_TELEMALARM3,
    MUSIC_TELEMALARM4,
    MUSIC_TELEMALARM5,
    MUSIC_TELEMALARM6,
    MUSIC_INACTIVITY_ALARM,
    MUSIC_TOTAL,	// Indicate total number of system music entries
};

#if HAS_EXTENDED_AUDIO
#define MAX_VOICEMAP_ENTRIES 400 // arbitraty chosen
#ifdef _DEVO12_TARGET_H_ // Check for Devo12 screen
#define MAX_VOICE_LABEL 35 // limit label length due to limited screen width
#else
#define MAX_VOICE_LABEL 26
#endif
#define VOICE_UNIT_OFFSET 130
#define CUSTOM_ALARM_ID 200 // start of custom MP3 IDs
#define VOICE_DEC_SEP 110  // MP3 ID of DECSEP = 110 + MUSIC_TOTAL
#define NUM_STICKS 4
#define NUM_AUX_KNOBS	(INP_HAS_CALIBRATION - NUM_STICKS)	// Exclude sticks
#define NUM_SWITCHES (NUM_INPUTS - INP_HAS_CALIBRATION)
#define MODEL_CUSTOM_ALARMS (NUM_SWITCHES + NUM_AUX_KNOBS * 2 + NUM_TIMERS + TELEM_NUM_ALARMS + NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS)

enum AudioPlayers {
  AUDIO_NONE = 0,	// Just use Tx beeps.
  AUDIO_AUDIOFX,	// Adafruit AUDIOFX board
  AUDIO_DF_PLAYER,	// DF Player Mini
  AUDIO_LAST
};

enum AudioDevices {
    AUDDEV_UNDEF = 0,	// Undefined (Use EXTAUDIO if extended-audio is enabled, otherwise use BUZZER)
    AUDDEV_ALL,
    AUDDEV_BUZZER,
    AUDDEV_EXTAUDIO,
    AUDDEV_LAST
};

enum {
    VOICE_UNIT_NONE = 0,
    VOICE_UNIT_MINUS,
    VOICE_UNIT_TEMP,
    VOICE_UNIT_VOLT,
    VOICE_UNIT_RPM,
    VOICE_UNIT_AMPS,
    VOICE_UNIT_ALTITUDE,
    VOICE_UNIT_GFORCE,
    VOICE_UNIT_KMH,
    VOICE_UNIT_MPS,
    VOICE_UNIT_MPH,
    VOICE_UNIT_KNOTS,
    VOICE_UNIT_METERS,
    VOICE_UNIT_CELSIUS,
    VOICE_UNIT_FAHRENHEIT,
    VOICE_UNIT_PERCENT,
    VOICE_UNIT_WATT,
    VOICE_UNIT_DB,
    VOICE_UNIT_SECONDS,
    VOICE_UNIT_MINUTES,
    VOICE_UNIT_HOURS,
    VOICE_UNIT_TIME, // used for auto-splitting to hours/minutes/seconds
    VOICE_UNIT_TOTAL
};

struct ButtonVoice {
    u16 on;             // Music to be played when button is On
    u16 off;
    u16 music;            // Music to be played when button is Off
};

struct CustomVoice {
    u16 music;
};

struct  Voice {
    struct CustomVoice switches[NUM_SWITCHES];	//Switch array to point to music file number, no pots
    struct CustomVoice telemetry[TELEM_NUM_ALARMS]; //Telemetry Alarm array to point to music file number
    struct CustomVoice timer[NUM_TIMERS]; //Timer Alarm array to point to music file number
    struct CustomVoice mixer[NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS];
    struct ButtonVoice buttons[NUM_TX_BUTTONS];	//Button array to point to music file number
#if NUM_AUX_KNOBS
    struct CustomVoice aux[NUM_AUX_KNOBS * 2]; //two per knob for up and down
#endif
};

u16 voice_map_entries;

struct VoiceMap {
    u16 id;
    u16 duration;
#if HAS_MUSIC_CONFIG
    char label[MAX_VOICE_LABEL];
#endif
};

struct VoiceMap voice_map[MAX_VOICEMAP_ENTRIES];

u16 MUSIC_GetTelemetryAlarm(enum Music music);
u16 MUSIC_GetTimerAlarm(enum Music music);
void MUSIC_PlayValue(u16 music, s32 value, u8 unit, u8 prec);

#endif //HAS_EXTENDED_AUDIO

void MUSIC_Beep(char* note, u16 duration, u16 interval, u8 count);

void MUSIC_Play(u16 music);

#endif
