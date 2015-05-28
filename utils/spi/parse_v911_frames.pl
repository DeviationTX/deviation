#!/usr/bin/env perl
use strict;
use warnings;

my $start = shift(@ARGV);
my $found = 0;
my $last_frame = 0;
my $frame_start = 0;
my @data = ();
my $lsb = 1;
my $tmp = 0;
$_ = <>;
while(<>) {
    chomp;
    my($time, $framenum, $data) = split(/,/, $_);
    $data = hex($data);
    $found = 1 if(int($framenum) == $start);
    next unless($found);
    if($framenum > $last_frame + 1) {
        $last_frame = $framenum;
        $frame_start = $time;
        @data = ();
    }
    if($framenum == $last_frame) {
        if($lsb) {
            $tmp = $data;
            $lsb = 0;
        } else {
            push @data, sprintf("%04x", ($data << 8) + $tmp);
            $lsb = 1;
        }
    } else {
        $_ = <>;
        ($time, $framenum, $data) = split(/,/, $_);
        my $channel = hex($data);
        printf("%15s(Channel: %02x), @data\n", $frame_start, $channel);
    }
}
