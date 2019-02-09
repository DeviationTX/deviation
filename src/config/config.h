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

#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <stddef.h>

enum {
    TYPE_U8 = 0,
    TYPE_U16 = 1,
    TYPE_U32 = 3,

    TYPE_S8 = 4,
    TYPE_S16 = 5,
    TYPE_S32 = 7,

    TYPE_STR_LIST = 8,
    TYPE_STR_CALL = 9,

    TYPE_COLOR = 10,
    TYPE_FONT = 11,
    TYPE_SOURCE = 12,
    TYPE_BUTTON = 13,
};

struct struct_map {const char *str;  u16 offset;};
#define ARRAYSIZE(x)  (sizeof(x) / sizeof(x[0]))
#define OFFSET(s, v) (offsetof(s, v) | ((sizeof(((s*)0)->v) - 1) << 12))
#define OFFSETS(s, v) (offsetof(s, v) | ((sizeof(((s*)0)->v) + 3) << 12))
#define OFFSET_COL(s, v) (offsetof(s, v) | (TYPE_COLOR << 12))
#define OFFSET_FON(s, v) (offsetof(s, v) | (TYPE_FONT << 12))
#define OFFSET_STRLIST(s, v, StrList, StrListSize) \
    (offsetof(s, v) | (TYPE_STR_LIST << 12)) }, { (const char*)StrList, StrListSize
#define OFFSET_STRCALL(s, v, StrCallback, N) \
    (offsetof(s, v) | (TYPE_STR_LIST << 12)) }, { (const char*)StrCallback, N
#define OFFSET_SRC(s, v) (offsetof(s, v) | (TYPE_SOURCE << 12))
#define OFFSET_BUT(s, v) (offsetof(s, v) | (TYPE_BUTTON << 12))

int assign_int(void* ptr, const struct struct_map *map, int map_size,
    const char* name, const char* value);
int write_int(void* ptr, const struct struct_map *map, int map_size, FILE* fh);

#define DEFAULTS_ZERO   (const u16*)0x01
#define DEFAULTS_ALWAYS (const u16*)0x02
int write_int2(void* ptr, const struct struct_map *map, int map_size,
            const u16* defaults, int defaults_size, FILE* fh);

#endif
