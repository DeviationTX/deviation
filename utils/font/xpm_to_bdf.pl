#!/usr/bin/perl
use strict;
use warnings;

my $descent = 2;
my $width = 12;
my $height = 18;

my $padding = 8 - ($width % 8);
my $bytes = 2* ($width+$padding)/8;
my %font;

# This is mapping from the IA911 font ROM to and ASCII table
my @map = (
# 0x00   0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f
  0x30,  0x31,  0x32,  0x33,  0x34,  0x35,  0x36,  0x37,  0x38,  0x39,  0x3a,  0x3c,  0x3e,  0x2d,  0x2e,  0x2c,  #0x00
  0x20,  0x41,  0x42,  0x43,  0x44,  0x45,  0x46,  0x47,  0x48,  0x49,  0x4a,  0x4b,  0x4c,  0x4d,  0x4e,  0xdb,  #0x10
  0x50,  0x51,  0x52,  0x53,  0x54,  0x55,  0x56,  0x57,  0x58,  0x59,  0x5a,  0x00,  0x00,  0x00,  0x00,  0x00,  #0x20
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,0x2192,0x2190,0x2191,0x2193,  0xdc,  #0x30
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  #0x40
  0x3f,  0x61,  0x62,  0x63,  0x64,  0x65,  0x66,  0x67,  0x68,  0x69,  0x6a,  0x6b,  0x6c,  0x6d,  0x6e,  0x6f,  #0x50
  0x70,  0x71,  0x72,  0x73,  0x74,  0x75,  0x76,  0x77,  0x78,  0x79,  0x7a,  0x3b,  0x2a,  0x2f,  0xb7,0x2663,  #0x60
  0x00,  0x00,0x263c,0x263a,0x263b,0x2666,0x266a,  0x00,  0x00,  0x00,  0x00,  0x00,  0x5d,  0x5b,  0x00,  0x00,  #0x70
  0x4f,                                                                                                           #0x80 (fake 'O')
);
sub read_fon {
   my($file) = @_;
   my @lines;
   open my $fh, "<", $file;
   while(<$fh>) {
       chomp;
       my(@cols) = split(//, $_);
       my $val = 0;
       foreach my $pxl (@cols) {
           $val = ($val << 1) | ($pxl eq "*" ? 1: 0);
       }
       $val = $val << $padding;
       push @lines, $val;
   }
   return \@lines;
}

sub make_bdf {
    print "STARTFONT 2.1\n";
    print "FONT -Deviation--$height-" . ($height*10) . "-75-75\n";
    print "SIZE $height 75 75\n";
    print "FONTBOUNDINGBOX $width $height 0 -$descent\n";
    print "STARTPROPERTIES 8\n";
    print "FONT_NAME \"Deviation\"\n";
    print "FONT_ASCENT " . ($height - $descent) . "\n";
    print "FONT_DESCENT $descent\n";
    print "PIXEL_SIZE $height\n";
    print "POINT_SIZE " . ($height * 10) . "\n";
    print "RESOLUTION_X 75\n";
    print "RESOLUTION_Y 75\n";
    print "RESOLUTION 75\n";
    print "ENDPROPERTIES\n";
    print "CHARS " . scalar(keys(%font)) . "\n";
    foreach my $chr (keys %font) {
        my $enc = hex($chr);
        #if ($map[$enc]) {
        #    $enc = $map[$enc];
        #} else {
        #    die "Couldn't map $chr\n";
        #}
        printf "STARTCHAR U+%04x\n", $enc;
        print "ENCODING $enc\n";
        print "SWIDTH 500 0\n";
        print "DWIDTH 0 0\n";
        print "BBX $width $height 0 -$descent\n";
        print "BITMAP\n";
        foreach my $row (@{ $font{$chr} }) {
            printf("%0${bytes}x\n", $row);
        }
        print "ENDCHAR\n";
    }
    print "ENDFONT\n";
}

while(@ARGV) {
    my $file = shift @ARGV;
    my ($chr) = ($file =~ /([^\/]*)\.fon$/);
    $font{$chr} = read_fon($file);
}
make_bdf();
