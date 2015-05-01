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
            item.append(self.format_data(i, data[idx:]))
            idx += size
        if idx != self.capture_size:
            print "Size mismatch"
            sys.exit(1)
        self.data.append(item)
    def to_model(self, value):
        timers = ["Timer1", "Timer2", "Timer3", "Timer4"]
        telem_volt = ["Volt1", "Volt2", "Volt3"]
        telem_temp = ["Temp1(C)", "Temp2(C)", "Temp3(C)", "Temp4(C)"]
        telem_rpm  = ["RPM1", "RPM2"]
        telem_extra_items = 49
        telem_extra = []
        for i in range(telem_extra_items):
            telem_extra.append("TELEM_" + `i`)
        inp    = []
        outch  = ["Channel1", "Channel2", "Channel3", "Channel4",
                  "Channel5", "Channel6", "Channel7", "Channel8",
                  "Channel9", "Channel10", "Channel11", "Channel12"]
        virtch = ["Virt1", "Virt2", "Virt3", "Virt4", "Virt5",
                  "Virt6", "Virt7", "Virt8", "Virt9", "Virt10"]
        ppm    = ["PPM1", "PPM2", "PPM3", "PPM4", "PPM5", "PPM6", "PPM7", "PPM8"]
        gps_loc = ["Latitude,Longitude"]
        gps_alt = ["Altitude(m)"]
        gps_speed = ["Velocity(m/s)"]
        gps_time  = ["GPSTime"]
        rtc    = []
        if value == 0x06:
            self.model = "Devo6"
            inp = ["AIL", "ELE", "THR", "RUD", "DR0", "DR1", "GEAR0", "GEAR1",
                   "MIX0", "MIX1", "MIX2", "FMODE0", "FMODE1", "FMODE2"]
        elif value == 0x08:
            self.model = "Devo8"
            inp = ["AIL", "ELE", "THR", "RUD", "RUD_DR0", "RUD_DR1", "ELE_DR0", "ELE_DR1",
                   "AIL_DR0", "AIL_DR1", "GEAR0", "GEAR1",
                   "MIX0", "MIX1", "MIX2", "FMODE0", "FMODE1", "FMODE2"]
        elif value == 0x0a:
            self.model = "Devo10"
            inp = ["AIL", "ELE", "THR", "RUD", "AUX4", "AUX5", "RUD_DR0", "RUD_DR1",
                   "ELE_DR0", "ELE_DR1", "AIL_DR0", "AIL_DR1", "GEAR0", "GEAR1",
                   "MIX0", "MIX1", "MIX2", "FMODE0", "FMODE1", "FMODE2"]
        elif value == 0x0c:
            self.model = "Devo12"
            inp = ["AIL", "ELE", "THR", "RUD", "AUX2", "AUX3","AUX4", "AUX5", "AUX6", "AUX7",
                   "RUD_DR0", "RUD_DR1", "RUD_DR2", 
                   "ELE_DR0", "ELE_DR1", "ELE_DR2",
                   "AIL_DR0", "AIL_DR1", "AIL_DR2",
                   "GEAR0", "GEAR1",
                   "MIX0", "MIX1", "MIX2", "FMODE0", "FMODE1", "FMODE2",
                   "HOLD0", "HOLD1", "TRAIN0", "TRAIN1"]
            rtc = ["Clock"]
        elif value == 0x7e:
            self.model = "Devo7e"
            inp = ["AIL", "ELE", "THR", "RUD", "HOLD0", "HOLD1", "FMODE0", "FMODE1"]
        else:
            return 0
        self.TIMER        = 0
        self.TELEM_VOLT = self.TIMER      + len(timers)
        self.TELEM_TEMP = self.TELEM_VOLT + len(telem_volt)
        self.TELEM_RPM  = self.TELEM_TEMP + len(telem_temp)
        self.TELEM_EXTRA= self.TELEM_RPM  + len(telem_rpm)
        self.INPUT      = self.TELEM_EXTRA+ len(telem_extra)
        self.CHAN       = self.INPUT      + len(inp)
        self.PPM        = self.CHAN       + len(outch) + len(virtch)
        self.GPS_LOC    = self.PPM        + len(ppm)
        self.GPS_ALT    = self.GPS_LOC    + len(gps_loc)
        self.GPS_SPEED  = self.GPS_ALT    + len(gps_alt)
        self.GPS_TIME   = self.GPS_SPEED  + len(gps_speed)
        self.RTC        = self.GPS_TIME   + len(gps_time)
        self.max_elem   = self.RTC        + len(rtc)

        self.elem_names = timers + telem_volt + telem_temp + telem_rpm + telem_extra \
                          + inp + outch + virtch + ppm + gps_loc + gps_alt + gps_speed + gps_time + rtc
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
            return 4
        if idx < self.GPS_LOC:
            return 1
        if idx == self.GPS_LOC:
            return 8
        return 4
    def format_data(self, type, data):
        if type < self.TELEM_VOLT: #Timer
            value = (data[1] << 8) | data[0]
            return "%02d:%02d" % (value / 60, value % 60)
        if type < self.TELEM_TEMP: #Telem Volt
            value = (data[1] << 8) | data[0]
            return "%d.%d" % (value / 10, value % 10)
        if type < self.INPUT:      #Telem Temp
            #                      #Telem RPM
            value = (data[1] << 8) | data[0]
            return "%d" % (value)
        if type < self.GPS_LOC:    #Inputs
            #                      #Channels
            #                      #PPM
            return "%d" % (data[0] - 0x100 if (data[0] & 0x80) else data[0])
        if type == self.GPS_LOC:
            value = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)
            h = value / 1000 / 60 / 60;
            m = (value - h * 1000 * 60 * 60) / 1000 / 60;
            s = (value - h * 1000 * 60 * 60 - m * 1000 * 60) / 1000;
            ss = value % 1000;
            str = "%03d %02d %02d.%03d" % (h, m, s, ss)
            value = data[4] + (data[5] << 8) + (data[6] << 16) + (data[7] << 24)
            h = value / 1000 / 60 / 60;
            m = (value - h * 1000 * 60 * 60) / 1000 / 60;
            s = (value - h * 1000 * 60 * 60 - m * 1000 * 60) / 1000;
            ss = value % 1000;
            return "%s,%03d %02d %02d.%03d" % (str, h, m, s, ss)
        if type == self.GPS_ALT or type == self.GPS_SPEED:
            value = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)
            return "%d.%03d" % (value / 1000, value % 1000)
        if type == self.GPS_TIME:
            value = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)
            year  = 2000 + ((value >> 26) & 0x3F)
            month = (value >> 22) & 0x0F;
            day   = (value >> 17) & 0x1F
            hour  = (value >> 12) & 0x1F
            min   = (value >>  6) & 0x3F
            sec   = (value >>  0) & 0x3F
            return "%02d:%02d:%02d %04d-%02d-%02d" % (hour, min, sec, year, month, day)
        if type == self.RTC:
            value = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)
            DAYSEC = (60*60*24)
            daysInYear = [ [ 0,31,59,90,120,151,181,212,243,273,304,334,365],
                           [ 0,31,60,91,121,152,182,213,244,274,305,335,366] ]

            days = value / DAYSEC;
            year = (4*days) / 1461; # = days/365.25
            leap = 1 if year % 4 == 0 else 0
            days = year * 365 + year / 4
            days -= 1 if (year != 0 and days > daysInYear[leap][2]) else 0  #leap year correction for RTC_STARTYEAR
            month = 0;
            for month in range(0, 12):
                if days < daysInYear[leap][month + 1]:
                    break;
            day = days - daysInYear[leap][month]
            month += 1
            sec = value % 60
            min = (value / 60) % 60
            hour = (value / 3600) % 24
            return "%02d:%02d:%02d %04d-%02d-%02d" % (hour, min, sec, 2012 + year, month, day)
        return "Unknown(%d)" %(data[0])

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
            info[-1].add_elem(data[idx+1:])
            idx += info[-1].capture_size+1
            continue
        if data[idx] != 0x02:
            printf("Cannot handle API version 0x%02x\n", data[idx])
            return info
        info.append(Capture(data[idx:]))
        idx += info[-1].header_size
        #printf("Header size: %d max_elem: %d data size: %d\n", info[-1].header_size, info[-1].max_elem, info[-1].capture_size)

main()
