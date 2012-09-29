#!/usr/bin/perl
use strict; 
use warnings;

use Encode;
my %chars;
while(<>) {
    my $str = decode("UTF-8", $_);
    foreach my $c (split(//, $str)) {
        $chars{ord($c)} = $c
    }
}
foreach (sort {$a <=> $b} keys(%chars)) {
    my $utf8 = encode("UTF-8", $chars{$_});
    my @u8;
    foreach my $c (split(//, $utf8)) {
        push @u8, sprintf("%02x", ord($c));
    }
    printf "%#x: $chars{$_} (@u8)\n", $_;
}
