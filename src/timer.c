/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "target.h"
#include "timer.h"
#include "config/model.h"

static u8 timer_state[NUM_TIMERS];
static u32 timer_val[NUM_TIMERS];
static u32 last_time[NUM_TIMERS];

void TIMER_SetString(char *str, u16 time)
{
    u8 h = time / 3600;
    u8 m = (time - h*3600) / 60;
    u8 s = time -h*3600 - m*60;
    sprintf(str, "%02d:%02d:%02d", h, m, s);
}

void TIMER_StartStop(u8 timer)
{
    timer_state[timer] ^= 1;
    if(timer_state[timer]) {
        last_time[timer] = CLOCK_getms();
    }
}

void TIMER_Reset(u8 timer)
{
    timer_state[timer] = 0;
    if (Model.timer[timer].type == TIMER_STOPWATCH) {
        timer_val[timer] = 0;
    } else {
        timer_val[timer] = Model.timer[timer].timer * 1000;
    }
}

u16 TIMER_GetValue(u8 timer)
{
    return timer_val[timer] / 1000;
}

void TIMER_Init()
{
    u8 i;
    for (i = 0; i < NUM_TIMERS; i++)
        TIMER_Reset(i);
}

void TIMER_Update()
{
    u8 i;
    u32 t = CLOCK_getms();
    for (i = 0; i < NUM_TIMERS; i++) {
        if (Model.timer[i].type == TIMER_COUNTDOWN && ! timer_val[i])
            continue;
        s16 *raw = MIX_GetInputs();
        if (Model.timer[i].src) {
            s16 val = raw[MIX_SRC(Model.timer[i].src)];
            if (MIX_SRC_IS_INV(Model.timer[i].src))
                val = -val;
            u8 new_state = (val - CHAN_MIN_VALUE > (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 20) ? 1 : 0;
            if (new_state != timer_state[i]) {
                if (new_state)
                    last_time[i] = t;
                timer_state[i] = new_state;
            }
         }
        if (timer_state[i]) {
            u32 delta = t - last_time[i];
            if (Model.timer[i].type == TIMER_STOPWATCH) {
                timer_val[i] += delta;
            } else {
                if (timer_val[i] > delta) {
                    timer_val[i] -= delta;
                }else {
                    timer_val[i] = 0;
                    timer_state[i] = 0;
                    MUSIC_Play(MUSIC_ALARM1 + i);
                }
            }
            last_time[i] = t;
        }
    }
}
