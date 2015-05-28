#!/usr/bin/env perl
use strict;
use warnings;
use Getopt::Long;
my $all = 0;
my $starttime = 0;
my $telem = 1;
my $mux = 0;
GetOptions("mux"=> \$mux, "all" => \$all, "time=f" => \$starttime);
$_ = <>;
my $ok = 0;
my $last_frame = -1;
my $cmd = 0;
my $start = 0;
my $msb_first = 0;
my @data = ();

while(<>) {
    my($time, $framenum, $data, $din) = (/^(\S+),(\S+),(\S+),(\S*)/);
    next unless($time && $framenum && $data);
    $time -= $starttime;
    $data = hex($data);
    $din = hex($din);
    if($framenum != $last_frame) {
        if(@data) {
            if(($data & 0x3f) >= 0x20 && ($data & 0x3f) <= 0x25) {
                if (sprintf("%02x", $data) eq $data[0]) {
                    $last_frame = $framenum;
                    #$msb_first = 1;
                    next;
                }
            }
            printf("%15.6f%s @data\n", $start, $ok == 1 ? ":" : "#");
        }
        @data = ();
        $last_frame = $framenum;
        $start = $time;
        if ($all) {
            $ok = ($data & 0x80) ? 1 : 2;
        } else {
            if ( grep {($data & 0x3f) == $_} (0x00, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25)) {
                $ok = ($data & 0x80) ? 1 : 2;
            } else {
                $ok = 0;
            }
        }
    }
    if ($ok) {
        my $d = (! @data || ($ok == 1 || $mux)) ? $data : $din;
        if($msb_first) {
            splice @data, 1, 0, sprintf("%02x", $d);
            $msb_first = 0;
        } else {
            push @data, sprintf("%02x", $d);
        }
    }
}
