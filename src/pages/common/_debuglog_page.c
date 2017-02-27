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

ctassert((DEBUG_WINDOW_SIZE < 255), debug_window_size_too_big);
#define NUM_ROWS 20
#define NEXT_PTR(ptr) ((ptr == logstr + DEBUG_WINDOW_SIZE -1) ? logstr : ptr+1)
#define PREV_PTR(ptr) ((ptr == logstr) ? logstr + DEBUG_WINDOW_SIZE - 1 : ptr-1)

static struct debuglog_obj * const gui = &gui_objs.u.debuglog;
static volatile char logstr[DEBUG_WINDOW_SIZE];
static volatile char *wptr = logstr;
static volatile char *rptr = logstr;
static u8 line_pos[NUM_ROWS];
static u32 changed = 0;

static const char *str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    unsigned idx = (long)data;
    if (idx == 255) {
        return "";
    }
    const char *dptr = (char *)&logstr[line_pos[idx]];
    const char *w = (char *)wptr;
    sprintf(tempstring, "%d:", (long)data);
    char *ptr = tempstring+strlen(tempstring);
    while(dptr != w && *dptr != 0) {
        *ptr++ = *dptr++;
    }
    *ptr = 0;
    return tempstring;
}

static void find_line_ends()
{
    const char *r = (char *)rptr;
    const char *w = (char *)wptr;
    if (w == r) {
        memset(line_pos, 255, NUM_ROWS);
        return;
    }
    w = (char *)PREV_PTR(w);
    const char *prev_w = (char *)PREV_PTR(w);
    for (int i = 0; i < NUM_ROWS; i++) {
        while (*prev_w != 0) {
            if (w == r) {
                line_pos[i] = r - logstr;
                memset(line_pos+i+1, 255, NUM_ROWS-i-1);
                return;
            }
            w = prev_w;
            prev_w = (char *)PREV_PTR(w);
        }
        line_pos[i] = w - logstr;
        w = prev_w;
        prev_w = (char *)PREV_PTR(w);
    }
}

void PAGE_DebuglogEvent()
{
    if(changed) {
        changed = 0;
        find_line_ends();
        for (int i = 0; i < DEBUG_LINE_COUNT; i++) {
            GUI_Redraw(&gui->line[i]);
        }
    }
}

void DEBUGLOG_Putc(char c)
{
    volatile char *next_wptr = NEXT_PTR(wptr);
    if (rptr == next_wptr) {
        volatile char *next_rptr = NEXT_PTR(rptr);
        while (*rptr != 0) {
            rptr = next_rptr;
            next_rptr = NEXT_PTR(rptr);
        }
        rptr = next_rptr;
    }
    *wptr = c;
    if (*wptr == '\n') {
        *wptr = 0;
    }
    wptr = next_wptr;
    changed = 1;
}
void DEBUGLOG_AddStr(const char *str)
{
    unsigned len = strlen(str)+1;
    if (*str == '3' && *(str+1) == '3') {
        printf("here\n");
    }
    while (len) {
        DEBUGLOG_Putc(*str++);
        len--;
    }
}
