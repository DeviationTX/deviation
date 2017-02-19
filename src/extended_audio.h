#ifndef _EXTENDED_AUDIO_H_
#define _EXTENDED_AUDIO_H_

#ifdef HAS_EXTENDED_AUDIO

#define AUDIO_QUEUE_LENGTH 20

int AUDIO_Play(u16 music);
void AUDIO_SetVolume(void);
void AUDIO_CheckQueue(void);
void AUDIO_AddQueue(u16 music);

 u16 audio_queue[AUDIO_QUEUE_LENGTH]; //arbitraty chosen, do we need more?
 u8 next_audio;
 u8 num_audio;

#endif
#endif
