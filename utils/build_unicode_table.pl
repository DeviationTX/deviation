#!/usr/bin/env perl
use strict; 
use warnings;

use utf8;
use Encode;
use Getopt::Long;

binmode(STDOUT, ":utf8");

my $font;
my %fontchars;
GetOptions("font=s" => \$font);

if ($font) {
    open my $fh, "<", $font;
    while(<$fh>) {
        if(/^ENCODING\s+(\d+)/) {
            $fontchars{int($1)} = 1;
        }
    }
}

my %chars;
for my $f (@ARGV) {
    open my $fh, "<", $f;
    my $line = 0;
    while(<$fh>) {
        $line++;
        my $str = decode("UTF-8", $_);
        $str =~ s/^\x{FEFF}//;
        chomp($str);
        foreach my $c (split(//, $str)) {
            my $c_int = ord($c);
            if (! $chars{$c_int}) {
                if ($font && ! $fontchars{$c_int}) {
                    printf "$f($line) Missing %#x ($c) from $str\n", $c_int;
                }
                $chars{ord($c)} = $c
            }
        }
    }
}
foreach (sort {$a <=> $b} keys(%chars)) {
    next if ($font && $fontchars{$_});
    my $utf8 = encode("UTF-8", $chars{$_});
    my @u8;
    foreach my $c (split(//, $utf8)) {
        push @u8, sprintf("%02x", ord($c));
    }
    print "Missing: " if ($font) ;
    printf "%#x: $chars{$_} (@u8)\n", $_;
}
