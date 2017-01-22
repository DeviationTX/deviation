#ifndef _MUSIC_H_
#define _MUSIC_H_

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

#if HAS_EXTENDED_AUDIO
#define NUM_STICKS	4
#define NUM_AUX_KNOBS	(INP_HAS_CALIBRATION - NUM_STICKS)	// Exclude sticks

enum {
    TELEM_UNIT_NONE = 0,
    TELEM_UNIT_TEMP,
    TELEM_UNIT_VOLT,
    TELEM_UNIT_RPM,
    TELEM_UNIT_AMPS,
    TELEM_UNIT_ALTITUDE,
    TELEM_UNIT_GFORCE,
    TELEM_UNIT_SECONDS,
};

struct AuxMusic {
    u16 up_state_music;              // Music to be played when Aux turns up
    u16 down_state_music;            // Music to be played when Aux turns down
};

struct ButtonMusic {
    u16 on_state_music;             // Music to be played when button is On
    u16 off_state_music;            // Music to be played when button is Off
};

struct {u16 music; u16 duration;} music_durations[80];

u16 MUSIC_GetDuration(u16 music);

void MUSIC_PlayValue(enum Music music, u32 value, u16 unit);
#endif

void MUSIC_Beep(char* note, u16 duration, u16 interval, u8 count);

void MUSIC_Play(enum Music music);


#endif
