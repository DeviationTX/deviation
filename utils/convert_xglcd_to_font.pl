#!/usr/bin/perl
# this script takes on stdin the ouput of the MikroElectronica GLCD FontCreator
# in 'mikroC' X-GLCD lib format
# The output is a font file compatible with the LCD routines

use warnings;
use strict;

my @chars;
my $max_width = 0;
my $max_height = 0;
my $array_w = 0;
my $array_h = 0;
my $name = "";
while(<>) {
    if(/(\S+?)(\d+)x(\d+)\[\] =/) {
        ($name, $array_w, $array_h) = ($1, $2, $3);
    }
    next unless(/\/\/ Code for char (.)/);
    my $char = ord($1);
    my @data;
    while(/0x(\S\S)/g) {
        push @data, hex($1);
    }
    if ($data[0] > $max_width) {
        $max_width = $data[0];
    }
    push @chars, [$char, @data];
}

my $bpc = int(($array_h - 1) / 8) + 1;
printf "/*\n";
printf " * Converted from X-GLCD by convert_xglcd_to_font.pl\n";
printf " * " . scalar(localtime()) .  "\n";
printf " *\n";
printf " * width       : %d\n", $max_width;
printf " * height      : %d\n", $array_h;
printf " * first char  : 0x%02x (%s)\n", $chars[0][0], chr($chars[0][0]);
printf " * last char   : 0x%02x (%s)\n", $chars[0][0] + scalar(@chars) - 1, chr($chars[0][0] + scalar(@chars) - 1);
printf " * proportional: 1\n";
printf " * bytes/col   : %d\n", $bpc;
printf " */\n\n";
printf "const uint8_t Font${name}_${array_h}[] = {\n";
my $count = 0;
printf "    //font widths\n   ";
foreach my $c (@chars) {
    printf " 0x%02x,", $c->[1];
    if(++$count == 10) {
        printf"\n   ";
        $count = 0;
    }
}
printf "\n    // font data\n";
foreach my $c (@chars) {
    printf "    // Char: 0x%02x(%s)\n", $c->[0], chr($c->[0]);
    if ($bpc > 1) {
        for (my $i = 0; $i < $c->[1]; $i++) {
            my($start, $end) = (2 + $i * $bpc, $i * $bpc + $bpc + 1);
            printf "    " . join ", ", map {sprintf "0x%02x", $_} @$c[$start..$end];
            printf ",\n";
        }
    } else {
        printf "    %s,\n", join ", ", map {sprintf "0x%02x", $_} @$c[2 .. $c->[1]+1];
    }
}
printf "\n};";
