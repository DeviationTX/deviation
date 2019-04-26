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

#include "common.h"
#include "rftools.h"
#include "target.h"

#ifndef MODULAR

#if SUPPORT_XN297DUMP
static FILE *fh;

struct Xn297dump xn297dump;

void RFTOOLS_DumpXN297Packet(u8 *packet) {
    fprintf(fh, "%d ", CLOCK_getms() & 0xFFFF);
    for (unsigned int i = 0 ; i < xn297dump.pkt_len ; ++i) {
        fprintf(fh, "%02X", *packet);
        packet++;
    }
    fprintf(fh, "\n");
}

void RFTOOLS_InitDumpLog(int enable) {
    if (enable) {
        fh = fopen("datalog.bin", "w");
        fempty(fh);
    } else {
        fclose(fh);
    }
}

#endif

#if SUPPORT_SCANNER
struct Scanner Scanner;
#endif

#endif  // MODULAR
