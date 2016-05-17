#!/usr/bin/env python

import os
import sys
import struct
from optparse import OptionParser

FILEOBJ_NONE    = 0x00
FILEOBJ_DIR     = 0x41
FILEOBJ_FILE    = 0x43
FILEOBJ_DELDIR  = 0xC1
FILEOBJ_DELFILE = 0xC1
START_SECTOR    = 0xFF

def main():
    usage = """
%prog [opts]
"""

    parser = OptionParser(usage=usage)
    parser.add_option("-c", "--create", action="store_true", dest="create", default=False,
                      help="create DEVOFS archive")
    parser.add_option("-x", "--extract", action="store_true", dest="extract",
                      help="extract DEVOFS archive")
    parser.add_option("-s", "--size", action="store", type="int", default=64, dest="size",
                      help="filesystem size in kB (default = 64)")
    parser.add_option("-i", "--invert", action="store_true", dest="invert", default=False,
                      help="DEVOFS archive is inverted")

    parser.add_option("-f", "--fs", action="store", dest="fs",
                      help="DEVOFS File")
    parser.add_option("-d", "--dir", action="store", dest="dir",
                      help="directory to read/write")
    (opt, args) = parser.parse_args()
    if not opt.dir:
        print "Must specify --dir"
        return
    if not opt.fs:
        print "Must specify --fs"
        return
    if opt.create == opt.extract:
        print "Must specify either --extract or --create"
        return
    if opt.create:
        data = read_dir(opt.dir);
	write_fs(opt.fs, data, opt.size, opt.invert)
    if opt.extract:
        data = bytearray(open(opt.fs, "rb").read())
        if opt.invert:
            data = bytearray(map(lambda x: 0xff-x, data))
        data = align_data(data)
	build_dirs(opt.dir, data)

def read_dir(dir):
    fs = []
    dirs = {}
    dirs[dir] = 0 # Root dir
    next_dir = 1
    
    for root, directories, filenames in os.walk(dir):
       for directory in directories:
            full_dir = os.path.join(root, directory)
            if not os.listdir(full_dir):
                continue
            dir_id = dirs[root]
            dirs[full_dir] = next_dir
            next_dir = next_dir + 1
            data = add_dir(directory, dirs[root], dirs[full_dir])
            if data:
                # print "DIR: " + full_dir
                fs += data
       for filename in filenames:
            data = add_file(filename, dirs[root], open(os.path.join(root, filename), "rb").read())
            if data:
                # print "FILE: " + os.path.join(root, filename)
                fs += data
    return fs

def add_dir(dir, parent, id):
    f = dir.split('.')
    ext = ""
    root = f[0]
    if (len(f) >= 2):
        ext = f[1]
    if (root == ""):
        return
    return struct.pack("B B 8s 3s B B B ", FILEOBJ_DIR, parent, root, ext, id, 0, 0)

def add_file(filename, parent, data):
    f = filename.split('.')
    ext = ""
    root = f[0]
    if (len(f) >= 2):
        ext = f[1]
    size = len(data)
    if (root == ""):
        return
    return struct.pack("B B 8s 3s B B B", FILEOBJ_FILE, parent, root, ext, size >> 16, 0xff & (size >> 8), size & 0xff) + data

def write_fs(filename, data, fs_size, invert):
    if len(data) > 1024 * fs_size - 4096:
        print "ERROR: Requested filesystem size: {} is too large for {}kB filesystem".format(len(data), fs_size)
        sys.exit(1)
    print "Filesystem will use {}kB of {}kB filesystem".format((len(data) + 512) / 1024, fs_size)
    f = open(filename, "wb")
    data.reverse() # Enables 'pop'
    next_sec = START_SECTOR

    for i in range(1024 * fs_size):
        if not data or i >= 1024 * fs_size - 4096:
            f.write(chr(0x00 if not invert else 0xff))
            continue
        if i % 4096 == 0:
            f.write(chr(next_sec if not invert else 0xff-next_sec))
            next_sec = 1
            continue
        c = data.pop()
        if invert:
            c = chr(0xff-ord(c))
        f.write(c)
    f.close()

def align_data(data):
    newdata = bytearray()
    pos = 0
    while pos < len(data):
        if data[pos] == 0xff:
            newdata = data[pos:]
            if pos > 0:
                newdata += data[0:pos-1]
            break
        pos += 4096
    data = bytearray()
    pos = 0
    while(pos < len(newdata)):
        data += newdata[pos+1:pos+4096]
        pos += 4096
    return data

def build_dirs(dir, data):
    pos = 0;
    dirs = {}
    dirs[0] = "" # Root dir
    while pos < len(data):
       type, parent_id, basename, ext, size1, size2, size3 = struct.unpack("B B 8s 3s B B B", data[pos:pos+16])
       pos += 16
       if type == 0x00:
           break
       filename = basename.split('\x00')[0]
       if ext[0] != '\x00':
           filename += "." + ext.split('\x00')[0]
       parent_dir = dirs[parent_id]
       if type == FILEOBJ_DIR:
           print "MKDIR: " + os.path.join(dir, parent_dir, filename)
           os.mkdir(os.path.join(dir, parent_dir, filename))
           dirs[size1] = os.path.join(parent_dir, filename)
           continue
       if type == FILEOBJ_DELDIR:
           print "DELETED DIR: " + os.path.join(dir, parent_dir, filename)
           continue
       size = (size1 << 16) + (size2 << 8) + size3
       filedata = data[pos:pos+size]
       pos += size
       if type == FILEOBJ_FILE:
           print "MKFILE ({}): {}".format(size, os.path.join(dir, parent_dir, filename))
           fh = open(os.path.join(dir, parent_dir, filename), "wb")
           fh.write(filedata)
           fh.close()
           continue
       if type == FILEOBJ_DELFILE:
           print "DELETED FILE ({}): {}".format(size, os.path.join(dir, parent_dir, filename))
           continue

main()
