#!/usr/bin/perl
use strict;
use warnings;

# This is a very crude tool that will convert a picture of a font character
# (or a row of several characters that are fixed space apart)
# and convert it into an xpm-line representation
#
# This script will only work well with highly maginfied images.  It was tested with
# images where each pixel of thefont character was ~13x13 pixels in the image
# Also, it has no auto-detection.  Spacing between pixels and between characters must be
# Specified below
# NOTE: the output is not actually XPM, but easily converted as needed
#       see xpm_to_bdf.pl and bdf_to_font.pl

my $rows = 18;      # The number of rows per character
my $cols = 12;      # The number of columns per character
my $w = 13.3;       # The average magnification of the image in X (spacing in real pixels for each font pixel)
my $h = 13;         # The magnification of the image in Y (spacing in real pixels for each font pixel)
my $off_h = 8;      # How many pixels in X from 0,0 to the center of teh 1st pixel
my $off_w = 6;      # How many pixels in Y from 0,0 to the center of teh 1st pixel
my $chsize = 170.2; # The average spacing between characters in a row (roughly: w * cols + char_spacing)

my @lines;

sub get_char {
    my ($offset) = @_;
    my $str = "";
    for (my $y = $off_h; $y < scalar(@lines); $y += $h) {
        my (@pixels) = split(//, $lines[$y]);
        #print "@pixels\n";
        for my $c (0 .. 11) {
            my $x = int($offset + $off_w + $c * $w);
            if ($pixels[$x]  eq " ") {
                $str .= "*";
            } else {
                $str .= " ";
            }
        }
        $str .= "\n";
        #print join("", @pixels) . "\n";
    }
    return $str;
}

sub get_char_id {
    my($name) = @_;
    my $id = -1;
    if($name =~ /row(\d+)/) {
        $id = 8 * ($1-1);
    }
    if($name =~ /off(\d+)/) {
        $id += $1;
    }
    die "bad name: $name\n" if($id == -1);
    return $id;
}

my $file = shift @ARGV;
my ($base) = ($file =~ /([^\/]+)\./);
my $xpm = "tmp.xpm";
system("convert -depth 1 $file $xpm");
open my $fh, "<", $xpm;
@lines = <$fh>;
close $fh;
my $charid = get_char_id($base);
splice(@lines, 0, 8);
for (my $i= 0; $i < 8; $i++) {
    my $offset = int($chsize * $i);
    last if ($offset > length($lines[3]));
    my $chr = get_char($offset);
    open $fh, ">", sprintf("%02x.fon", $charid++);
    print $fh $chr;
    close $fh;
    print "$chr\n";
}
