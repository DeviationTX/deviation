#ifndef _EXTENDED_AUDIO_H_
#define _EXTENDED_AUDIO_H_

#ifdef HAS_EXTENDED_AUDIO

#define AUDIO_QUEUE_LENGTH 20 // arbitraty chosen, do we need more?

void AUDIO_Init();
int AUDIO_Play(u16 music);
void AUDIO_SetVolume();
void AUDIO_CheckQueue();
int AUDIO_AddQueue(u16 music);
int AUDIO_VoiceAvailable();

extern u16 audio_queue[AUDIO_QUEUE_LENGTH];
extern u8 next_audio;
extern u8 num_audio;
extern u32 audio_queue_time;

#endif
#endif
