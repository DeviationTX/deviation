#!/usr/bin/env python

import os
import sys
from optparse import OptionParser

class Capture(object):
    def init(self):
        self.model = "None"
        self.rate  = "None"
        self.num_elem = 0
        self.max_elem = 0
        self.header_mask = []
        self.elem_names = []
        self.header_size = 3
        self.capture_size = 0
        self.data = []

        self.TIMER   = 0
        self.TELEM   = 0
        self.INPUT   = 0
        self.CHAN    = 0
        self.PPM     = 0
        self.GPSLOC  = 0
        self.GPSTIME = 0
        self.RTC     = 0
    def __init__(self, data):
        self.init()
        header_mask_size = self.to_model(data[1])
        self.to_rate(data[2])
        self.header_mask = data[3:3+header_mask_size]
        self.capture_size = self.parse_size(self.header_mask)
        self.header_size = header_mask_size + 3
    def add_elem(self, data):
        item = []
        idx = 0;
        for i in range(self.max_elem):
            if not (self.header_mask[(i / 8)] & (1 << (i % 8))):
                continue
            size = self.get_size(i)
            if size == 1:
                item.append(data[idx])
            elif size == 2:
                item.append(data[idx] + (data[idx+1] << 8))
            elif size == 4:
                item.append(data[idx] + (data[idx+1] << 8) + (data[idx+2] << 16) + (data[idx+3] << 24))
            elif size == 16:
                item.append(data[idx+0] + (data[idx+1] << 8) + (data[idx+2] << 16) + (data[idx+3] << 24))
                item.append(data[idx+4] + (data[idx+5] << 8) + (data[idx+6] << 16) + (data[idx+7] << 24))
                item.append(data[idx+8] + (data[idx+9] << 8) + (data[idx+10] << 16) + (data[idx+11] << 24))
                item.append(data[idx+12] + (data[idx+13] << 8) + (data[idx+14] << 16) + (data[idx+15] << 24))
            idx += size
        self.data.append(item)
    def to_model(self, value):
        timers = ["Timer1", "Timer2", "Timer3", "Timer4"]
        telem  = ["Volt1", "Volt2", "Volt3",
                  "Temp1", "Temp2", "Temp3", "Temp4",
                  "RPM1", "RPM2"]
        inp    = []
        outch  = ["Channel1", "Channel2", "Channel3", "Channel4",
                  "Channel5", "Channel6", "Channel7", "Channel8",
                  "Channel9", "Channel10", "Channel11", "Channel12"]
        virtch = ["Virt1", "Virt2", "Virt3", "Virt4", "Virt5",
                  "Virt6", "Virt7", "Virt8", "Virt9", "Virt10"]
        ppm    = ["PPM1", "PPM2", "PPM3", "PPM4", "PPM5", "PPM6", "PPM7", "PPM8"]
        gpsloc = ["Latitude,Longitude,Altitude,Velocity"]
        gpstime = ["GPSTime"]
        rtc    = []
        if value == 0x06:
            self.model = "Devo6"
            inp = ["AIL", "ELE", "THR", "RUD", "DR0", "DR1", "GEAR0", "GEAR1",
                   "MIX0", "MIX1", "MIX2", "FMODE1", "FMODE2", "FMODE3"]
        elif value == 0x08:
            self.model = "Devo8"
            inp = ["AIL", "ELE", "THR", "RUD", "RUD_DR0", "RUD_DR1", "ELE_DR0", "ELE_DR1",
                   "AIL_DR0", "AIL_DR1", "GEAR0", "GEAR1",
                   "MIX0", "MIX1", "MIX2", "FMODE1", "FMODE2", "FMODE3"]
        elif value == 0x0a:
            self.model = "Devo10"
            inp = ["AIL", "ELE", "THR", "RUD", "AUX4", "AUX5", "RUD_DR0", "RUD_DR1",
                   "ELE_DR0", "ELE_DR1", "AIL_DR0", "AIL_DR1", "GEAR0", "GEAR1",
                   "MIX0", "MIX1", "MIX2", "FMODE1", "FMODE2", "FMODE3"]
        elif value == 0x0c:
            self.model = "Devo12"
            inp = ["AIL", "ELE", "THR", "RUD", "AUX2", "AUX3","AUX4", "AUX5", "AUX6", "AUX7",
                   "RUD_DR0", "RUD_DR1", "RUD_DR2", 
                   "ELE_DR0", "ELE_DR1", "ELE_DR2",
                   "AIL_DR0", "AIL_DR1", "AIL_DR2",
                   "GEAR0", "GEAR1",
                   "MIX0", "MIX1", "MIX2", "FMODE1", "FMODE2", "FMODE3",
                   "HOLD0", "HOLD1", "TRAIN0", "TRAIN1"]
            rtc = ["Clock"]
        elif value == 0x7e:
            self.model = "Devo7e"
            inp = ["AIL", "ELE", "THR", "RUD", "HOLD0", "HOLD1", "FMODE0", "FMODE1"]
        else:
            return 0
        self.TIMER   = 0
        self.TELEM   = self.TIMER + len(timers)
        self.INPUT   = self.TELEM + len(telem)
        self.CHAN    = self.INPUT + len(inp)
        self.PPM     = self.CHAN  + len(outch) + len(virtch)
        self.GPSLOC  = self.PPM   + len(ppm)
        self.GPSTIME = self.GPSLOC + len(gpsloc)
        self.RTC     = self.GPSTIME + len(gpstime)
        self.max_elem = self.RTC + len(rtc)
        self.elem_names = timers + telem + inp + outch + virtch + ppm + gpsloc + gpstime + rtc
        return (7 + self.max_elem) / 8
    def to_rate(self, value):
        if value == 0:
            self.rate = "1 sec"
        elif value == 1:
            self.rate = "5 sec"
        elif value == 2:
            self.rate = "10 sec"
        elif value == 3:
            self.rate = "30 sec"
        elif value == 4:
            self.rate = "1 min"
    def get_size(self, idx):
        if idx < self.INPUT:
            return 2
        if idx < self.GPSLOC:
            return 1
        if idx == self.GPSLOC:
            return 16
        return 4
    def parse_size(self, data):
        self.num_elem = 0
        for i in range(self.max_elem):
            if data[(i / 8)] & (1 << (i % 8)):
                self.num_elem += self.get_size(i)
        return self.num_elem
    def write_csv(self):
        head = []
        for i in range(self.max_elem):
            if self.header_mask[(i / 8)] & (1 << (i % 8)):
                head.append(self.elem_names[i])
        out = [",".join(head)+"\n"]
        for d in self.data:
            out.append(",".join(str(x) for x in d) + "\n")
        return out
        
def printf(format, *args):
    sys.stdout.write(format % args)

def main():
    usage = """
%prog [-b|--bin <bin-file>] [opts]
"""
    parser = OptionParser(usage=usage)
    parser.add_option("-a", "--all", action="store_true", dest="all", default=False,
                      help="write all captures")
    parser.add_option("-b", "--bin", action="store", dest="bin",
                      help="input file to read")
    parser.add_option("-c", "--clear", action="store_true", dest="clear", default=False,
                      help="clear log")
    parser.add_option("-l", "--list", action="store_true", dest="list", default=False,
                      help="list available captures")
    parser.add_option("-o", "--out", action="store", dest="out",
                      help="output file")
    parser.add_option("-s", "--size", action="store", type="int", dest="size",
                      help="resize bin file to <value> bytes (this will erase the log)")
    parser.add_option("-w", "--write", action="store", type="int", dest="write", default=False,
                      help="write outcpature # <value>")
    (opt, args) = parser.parse_args()
    if not opt.bin:
        print "Must specify --bin"
        return
    if not os.path.exists(opt.bin):
        print "Could not locate bin file: " + opt.bin
        return
    if opt.clear:
        stat = os.stat(opt.bin)
        with open(opt.bin, 'r+') as of:
            of.write('\0' * stat.st_size)
            of.flush()
        of.close()
        return
    if opt.size:
        with open(opt.bin, 'w') as of:
            of.write('\0' * opt.size)
            of.flush
        of.close()
        return
    info = parse_file(opt.bin)
    if opt.list:
        for i in range(len(info)):
            printf("%3d: %10s (rate=%s): %5d samples (%d)\n", i+1,
                   info[i].model, info[i].rate, len(info[i].data), info[i].num_elem)
        return
    out = []
    if opt.write:
        out.extend(info[opt.write-1].write_csv())
    elif opt.all:
        for i in info:
            out.extend(i.write_csv())
    else:
        return
    if opt.out:
        with open(opt.out, 'w') as of:
            for o in out:
              of.write(o)
            of.close()
        return
    else:
        for o in out:
            printf("%s", o)
def parse_file(bin):
    data = []
    for i in open(bin,'rb').read() :
        data.append(ord(i))
    idx = 0
    size = 1
    info = []
    while(idx < len(data)):
        if data[idx] == 0x00: #finished parsing
            return info
        if data[idx] == 0xff:
            idx += info[-1].capture_size+1
            info[-1].add_elem(data[idx+1:])
            continue
        if data[idx] != 0x01:
            printf("Cannot handle API version 0x%02x\n", data[idx])
            return info
        info.append(Capture(data[idx:]))
        idx += info[-1].header_size
        #printf("Header size: %d max_elem: %d data size: %d\n", info[-1].header_size, info[-1].max_elem, info[-1].capture_size)

main()
