/*
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com/
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <math.h>
#include "common.h"

#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (250)
#ifdef NO_SOUND
void SOUND_SetFrequency(unsigned freq, unsigned volume) {(void)freq; (void)volume;}
void SOUND_Init() {}
void SOUND_Start(unsigned msec, u16(*next_note_cb)(), u8 vibrate) {
    (void)msec;
    (void)next_note_cb;
    (void)vibrate;
    printf("beep\n");
}
void SOUND_StartWithoutVibrating(unsigned msec, u16(*next_note_cb)()) {
    (void)msec;
    (void)next_note_cb;
    printf("beep\n");
}
void SOUND_Stop() {}
#else

#include "portaudio.h"

struct {
    PaStreamParameters outputParameters;
    PaStream *stream;

    float sine[TABLE_SIZE];
    int table_size;
    int phase;
    const int *ptr;
    int duration;
    int enable;
    char message[20];
} paData;

void SOUND_SetFrequency(unsigned freq, unsigned volume)
{
    int i;
    if (freq == 0) {
        paData.table_size = TABLE_SIZE;
        volume = 0;
    } else {
        paData.table_size = SAMPLE_RATE / freq;
    }
    for(i = 0; i < paData.table_size; i++)
        paData.sine[i] = (float)volume / 100.0 * sin( ((double)i/(double)paData.table_size) * M_PI * 2. );
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int paCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    float *out = (float*)outputBuffer;
    unsigned long i;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;
    u16(*next_note_cb)() = (u16(*)())userData;
    for (i=0; i<framesPerBuffer; i++ )
    {
        *out++ = paData.sine[paData.phase];
        *out++ = paData.sine[paData.phase];
        paData.phase+=1;
        paData.duration--;
        if (! paData.duration) {
            if (next_note_cb == NULL)
                return paComplete;
            u16 msec = next_note_cb();
            if(! msec)
                return paComplete;
            paData.duration = SAMPLE_RATE * (long)msec / 1000;
            paData.phase = 0;
        }
        if (paData.phase >= paData.table_size) {
            paData.phase -= paData.table_size;
        }
    }
    
    return paContinue;
}

void SOUND_Init()
{
    PaError err;
    memset(&paData, 0, sizeof(paData));
    err = Pa_Initialize();
    if( err != paNoError ) {
        printf("Sound initialization failed.  Disabling sound\n");
        Pa_Terminate();
        return;
    }
    paData.outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (paData.outputParameters.device == paNoDevice) {
      printf("Error: No default output device.\n");
      Pa_Terminate();
      return;
    }
    paData.outputParameters.channelCount = 2;       /* stereo output */
    paData.outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    paData.outputParameters.suggestedLatency =
              Pa_GetDeviceInfo( paData.outputParameters.device )->defaultLowOutputLatency;
    paData.outputParameters.hostApiSpecificStreamInfo = NULL;
    paData.enable = 1;
}

void SOUND_StartWithoutVibrating(unsigned msec, u16(*next_note_cb)())
{
    SOUND_Start(msec, next_note_cb, 0);
}

void SOUND_Start(unsigned msec, u16(*next_note_cb)(), u8 vibrate) {
    (void)vibrate;
    PaError err;
    if (! paData.enable)
        return;
    SOUND_Stop();
    paData.duration = SAMPLE_RATE * (long)msec / 1000;
    err = Pa_OpenStream(
              &paData.stream,
              NULL, /* no input */
              &paData.outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              paCallback,
              next_note_cb);
    if (err != paNoError) {
        printf("Sound Failed.  Disabling\n");
        paData.enable = 0;
        Pa_Terminate();
        return;
    }
    err = Pa_StartStream(paData.stream);
    if(err != paNoError) {
        printf("Sound Failed.  Disabling\n");
        paData.enable = 0;
        Pa_Terminate();
        return;
    }
}

void SOUND_Stop()
{
    Pa_StopStream( paData.stream );
}
#endif
