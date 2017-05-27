#!/usr/bin/env python

# Written by Antonio Galea - 2010/11/18
# Distributed under Gnu LGPL 3.0
# see http://www.gnu.org/licenses/lgpl-3.0.txt

import sys,struct,zlib,os
from optparse import OptionParser

DEFAULT_DEVICE="0x0483:0xdf11"

def named(tuple,names):
  return dict(zip(names.split(),tuple))
def consume(fmt,data,names):
  n = struct.calcsize(fmt)
  return named(struct.unpack(fmt,data[:n]),names),data[n:]
def cstring(string):
  return string.split('\0',1)[0]
def compute_crc(data):
  return 0xFFFFFFFF & -zlib.crc32(data) -1

def encrypt(data, offset):
    result = []
    for val in map(ord, data):
        if(val >= 0x80 and val <= 0xcf - offset):
            val += offset
        elif(val >= 0xd0 - offset and val < 0xd0):
            val -= (0x50 - offset)
        result += chr(val)
    return ''.join(result)
def decrypt(data, offset):
    result = []
    for val in map(ord, data):
        if(val >= 0x80 + offset and val <= 0xcf):
            val -= offset
        elif(val >= 0x80 and val < 0x80 + offset):
            val += (0x50 - offset)
        result += chr(val)
    return ''.join(result)

def parse(file,dump_images=False,crypt=0):
  print 'File: "%s"' % file
  data = open(file,'rb').read()
  crc = compute_crc(data[:-4])
  prefix, data = consume('<5sBIB',data,'signature version size targets')
  print '%(signature)s v%(version)d, image size: %(size)d, targets: %(targets)d' % prefix
  for t in range(prefix['targets']):
    tprefix, data  = consume('<6sBI255s2I',data,'signature altsetting named name size elements')
    tprefix['num'] = t
    if tprefix['named']:
      tprefix['name'] = cstring(tprefix['name'])
    else:
      tprefix['name'] = ''
    print '%(signature)s %(num)d, alt setting: %(altsetting)s, name: "%(name)s", size: %(size)d, elements: %(elements)d' % tprefix
    tsize = tprefix['size']
    target, data = data[:tsize], data[tsize:]
    for e in range(tprefix['elements']):
      eprefix, target = consume('<2I',target,'address size')
      eprefix['num'] = e
      print '  %(num)d, address: 0x%(address)08x, size: %(size)d' % eprefix
      esize = eprefix['size']
      image, target = target[:esize], target[esize:]
      if dump_images:
        out = '%s.target%d.image%d.bin' % (file,t,e)
        image = decrypt(image, crypt)
        open(out,'wb').write(image)
        print '    DUMPED IMAGE TO "%s"' % out
    if len(target):
      print "target %d: PARSE ERROR" % t
  suffix = named(struct.unpack('<4H3sBI',data[:16]),'device product vendor dfu ufd len crc')
  print 'usb: %(vendor)04x:%(product)04x, device: 0x%(device)04x, dfu: 0x%(dfu)04x, %(ufd)s, %(len)d, 0x%(crc)08x' % suffix
  if crc != suffix['crc']:
    print "CRC ERROR: computed crc32 is 0x%08x" % crc
  data = data[16:]
  if data:
    print "PARSE ERROR"

# see https://github.com/hughsie/fwupd/blob/master/docs/dfu-metadata-store.md
def build_metadata(keys):
  data = struct.pack('<2s','MD')
  data += struct.pack('b', len(keys))
  for key in keys:
    for item in [key, keys[key]]:
      data += struct.pack('b', len(item))
      data += struct.pack('<' + str(len(item)) + 's', item)
  return data

def build(file,targets,options):
  data = ''
  for t,target in enumerate(targets):
    tdata = ''
    for image in target['images']:
      tdata += struct.pack('<2I',image['address'],len(image['data']))
      tdata += encrypt(image['data'],options.crypt)
    tdata = struct.pack('<6sBI255s2I','Target',target['alt'],1,options.name,len(tdata),len(target['images'])) + tdata
    data += tdata
  data  = struct.pack('<5sBIB','DfuSe',1,len(data)+11,len(targets)) + data
  v,d=map(lambda x: int(x,0) & 0xFFFF, options.device.split(':',1))
  keys = {}
  keys['License'] = 'GPL-3.0'
  if options.crypt > 0:
    keys['CipherKind'] = 'DEVO'
  metadata = build_metadata(keys)
  data += metadata
  data += struct.pack('<4H3sB',options.version,d,v,0x011a,'UFD',16 + len(metadata))
  crc   = compute_crc(data)
  data += struct.pack('<I',crc)
  open(file,'wb').write(data)

if __name__=="__main__":
  usage = """
%prog [-d|--dump] infile.dfu
%prog {-b|--build} address:file.bin [-b address:file.bin ...] [{-D|--device}=vendor:device] outfile.dfu"""
  parser = OptionParser(usage=usage)
  parser.add_option("-b", "--build", action="append", dest="binfiles",
    help="build a DFU file from given BINFILES", metavar="BINFILES")
  parser.add_option("-D", "--device", action="store", dest="device",
    help="build for DEVICE, defaults to %s" % DEFAULT_DEVICE, metavar="DEVICE")
  parser.add_option("-d", "--dump", action="store_true", dest="dump_images",
    default=False, help="dump contained images to current directory")
  parser.add_option("-c", "--crypt", action="store", type="int", dest="crypt",
    default=0, help="use scramble offset of 'xx' (6, 7, 8, 10, 12)")
  parser.add_option("-a", "--alt", action="append", dest="alt",
    help="specifies alternate-seting")
  parser.add_option("-v", "--version", action="store", type="int", dest="version",
    default=0, help="specifies firmware version")
  parser.add_option("-n", "--name", action="store", dest="name",
    default='ST...', help="specifies firmware version")
  (options, args) = parser.parse_args()

  if options.binfiles and len(args)==1:
    target = []
    for arg in options.binfiles:
      try:
        address,binfile = arg.split(':',1)
      except ValueError:
        print "Address:file couple '%s' invalid." % arg
        sys.exit(1)
      try:
        address = int(address,0) & 0xFFFFFFFF
      except ValueError:
        print "Address %s invalid." % address
        sys.exit(1)
      if not os.path.isfile(binfile):
        print "Unreadable file '%s'." % binfile
        sys.exit(1)
      target.append({ 'address': address, 'data': open(binfile,'rb').read() })
    outfile = args[0]

    if not options.device:
      options.device = DEFAULT_DEVICE
    try:
      v,d=map(lambda x: int(x,0) & 0xFFFF, options.device.split(':',1))
    except:
      print "Invalid device '%s'." % options.device
      sys.exit(1)
    if not options.alt:
        options.alt = ["0"]
    if len(options.alt) != 1 and len(options.alt) != len(target):
      print "Number of alt devices (%d) must match number of targets(%d)" % (len(options.alt), len(target))
      sys.exit(1)
    targets = []
    if len(options.alt) == 1: 
      targets.append({'alt': int(options.alt[0]), 'images' : target})
    else:
      for t,image in enumerate(target):
        targets.append({'alt': int(options.alt[t]), 'images' : [image] })
    build(outfile,targets,options)
  elif len(args)==1:
    infile = args[0]
    if not os.path.isfile(infile):
      print "Unreadable file '%s'." % infile
      sys.exit(1)
    parse(infile, dump_images=options.dump_images,crypt=options.crypt)
  else:
    parser.print_help()
    sys.exit(1)
