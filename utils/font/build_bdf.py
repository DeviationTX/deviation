#!/usr/bin/env python
# -*- coding: utf-8 -*-
# -----------------------------------------------------------------------------
#
#  FreeType high-level python API - Copyright 2011 Nicolas P. Rougier
#  Distributed under the terms of the new BSD license.
#
# -----------------------------------------------------------------------------
'''
Glyph bitmap monochrome rendring
'''
from freetype import *
from optparse import OptionParser
import os

def main():
    target_size = 12
    max_ascent = 10
    space = 3
    name = "Deviation"
    fonts = [['./Ubuntu-C.ttf', 0],
             ['./fireflysung.ttf', 0],
             ['./wqy-zenhei.ttc', 0],
             ]
    char_set = [[0, 0x20, 0x7e],
                 [0, 0xa1, 0x10d],
                 [0, 0x0112, 0x0113],
                 [0, 0x012a, 0x012b],
                 [0, 0x0132, 0x0133],
                 [0, 0x014c, 0x014d],
                 [0, 0x0150, 0x0153],
                 [0, 0x0160, 0x0161],
                 [0, 0x016a, 0x016b],
                 [0, 0x0170, 0x0171],
                 [0, 0x0174, 0x0177],
                 [0, 0x017d, 0x017e],
                 [0, 0x0218, 0x021b],
                 [0, 0x0400, 0x045f],
                 [0, 0x1e80, 0x1e83],
                 [1, 0x4e00, 0x9fff]]
    usage = """
%prog [-s|--size <size>] [-a|--ascent <ascent>] [-b|--bdffile <file>] [-t|--ttf <ttf1,ttf2,...>]
"""
    parser = OptionParser(usage=usage)
    parser.add_option("-s", "--size", action="store", type="int", dest="size",
        default=False, help="Specify maximum height of font")
    parser.add_option("-a", "--ascent",  action="store", type="int", dest="ascent",
        default=False, help="Specify row # of baseline")
    parser.add_option("-b", "--bdffile", action="store", dest="bdffile",
        default = name+".bdf", help="BDF file to create")
    parser.add_option("-r", "--range", action="store", dest="range",
        default=False, help="Specify range of chars")
    parser.add_option("-t", "--ttf", action="append", dest="ttf",
        help="Input ttf files to use (order is important).  Append ':#' to force max font-size")
    parser.add_option("-d", "--debug", action="store_true", dest="debug",
        default=False, help="enable debug output")
    parser.add_option("-p", "--space", action="store", type="int", dest="space",
        default=False, help="size of space character")
    (options, args) = parser.parse_args()

    if options.ttf:
        fonts = []
        for ttf in options.ttf:
            f = ttf.rsplit(":", 1)
            if len(f) == 1:
                fonts.append([f[0], 0])
            else:
                fonts.append([f[0], int(f[1])])
    if options.ascent:
        max_ascent = options.ascent
    if options.size:
        target_size = options.size
    if options.space:
        space = options.space
    if options.range:
        char_set = []
        for r in options.range.split(","):
            (start, end) = r.split("-")
            char_set.append([0, int(start), int(end)])

    faces = []
    for (f, s) in fonts:
        if not os.path.exists(f):
            print "Could not locate " + f + "Skipping\n"
            continue
        print "Using TTF file: " + f
        if s == 0:
            s = target_size+1
        faces.append([Face(f), s])
    max_descent = target_size - max_ascent
    num_chars = 0
    chardata = ""
    for r in char_set:
        for c in range(r[1], r[2]+1):
            #for each character
            uc = unichr(c)
            face = 0
            for (f,s) in faces:
                if f.get_char_index(uc) != 0:
                    face = f
                    maxsize = s
                    break
            if face == 0:
                print "No character found for " + `c`
                continue
            #we have found a character
            for size in range(maxsize, target_size-8, -1):
                face.set_char_size( size*64 )
                face.load_char(uc, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO )
                rows   = face.glyph.bitmap.rows
                top    = face.glyph.bitmap_top
                ascent = top
                descent = rows - top
                if (r[0] == 0):
                    if (ascent > max_ascent or descent > max_descent):
                        continue
                else:
                    #Ignore character ascent
                    if (rows > size):
                        continue
                width  = face.glyph.bitmap.width
                bitmap = face.glyph.bitmap
                pitch  = face.glyph.bitmap.pitch
                if options.debug:
                    print "(%s %02d) %05d/%04x: w:%d, a:%d, d:%d p:%d" %(face.family_name, size, c, c, width, ascent, descent, pitch)
                if c == 32:
                    width = space
                num_chars += 1
                chardata += "STARTCHAR uni%04X\n" % (c)
                chardata += "ENCODING " + `c` + "\n"
                chardata += "SWIDTH 500 0\n"
                chardata += "DWIDTH 6 0\n"
                chardata += "BBX %d %d 0 %d\n" % (width, rows, top - rows)
                chardata += "BITMAP\n"
                idx = 0
                for i in bitmap.buffer:
                    if (idx % pitch) < int((width + 7) / 8):
                        chardata += "%02X" % (i)
                    idx += 1
                    if (idx % pitch) == 0:
                        chardata += "\n"
                chardata += "ENDCHAR\n"
                break

    data = ""
    data += "STARTFONT 2.1\n"
    data += "FONT -" + name + "--" + `target_size` + "-" + `target_size * 10` + "-75-75\n"
    data += "SIZE " + `target_size` + " 75 75\n"
    data += "FONTBOUNDINGBOX " + `target_size` + " " + `target_size` \
             + " 0 " + `max_ascent - target_size` + "\n"
    data += "STARTPROPERTIES 8\n"
    data += "FONT_NAME \"" + name + "\"\n"
    data += "FONT_ASCENT " + `max_ascent` + "\n"
    data += "FONT_DESCENT " + `target_size - max_ascent` + "\n"
    data += "PIXEL_SIZE " + `target_size` + "\n"
    data += "POINT_SIZE " + `target_size * 10` + "\n"
    data += "RESOLUTION_X 75\n"
    data += "RESOLUTION_Y 75\n"
    data += "RESOLUTION 75\n"
    data += "ENDPROPERTIES\n"
    data += "CHARS " + `num_chars` + "\n"
    data += chardata
    open(options.bdffile, "w").write(data)
    #print width
    #print rows, face.glyph.bitmap_top
    #print pitch
    #print bitmap.buffer
    #data = []
    #for i in range(bitmap.rows):
    #    row = []
    #    for j in range(bitmap.pitch):
    #        row.extend(bits(bitmap.buffer[i*bitmap.pitch+j]))
    #    data.extend(row[:bitmap.width])
    #Z = numpy.array(data).reshape(bitmap.rows, bitmap.width)
    #plt.imshow(Z, interpolation='nearest', cmap=plt.cm.gray)
    #plt.show()

main()
