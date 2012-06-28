#!/usr/bin/perl
use strict;
use warnings;
use Getopt::Long;
my $mfgid = "";
my @xor = (0, 0, 0, 0);
my $showchan = 0;
my $starttime = 0;
my $telem = 1;
GetOptions("mfgid=s" => \$mfgid, "chan" => \$showchan , "time=f" => \$starttime);
if($mfgid) {
    @xor = map {hex($_)} split(/ /, $mfgid);
    printf "Using mfgid: %02x %02x %02x %02x\n", @xor;
};

$_ = <>;
my $ok = 0;
my $last_frame = -1;
my $cmd = 0;
my $start = 0;
my @data = ();

while(<>) {
    my($time, $framenum, $data, $din) = (/^(\S+),(\S+),(\S+),(\S+)/);
    next unless($time && $framenum && $data);
    $time -= $starttime;
    $data = hex($data);
    $din = hex($din);
    if($framenum != $last_frame) {
        if(@data) {
            if($data[0] =~ /^[678ac][78bcd]$/) {
                for(my $i = 0; $i < 15; $i++) {
                    $data[$i + 1] = sprintf("%02x", hex($data[$i + 1]) ^ $xor[$i % 4]);
                }
            } elsif($data[0] =~ /^[678ac]a$/) {
                $data[13] = sprintf("%02x", hex($data[13]) ^ $xor[0]);
                $data[14] = sprintf("%02x", hex($data[14]) ^ $xor[1]);
                $data[15] = sprintf("%02x", hex($data[15]) ^ $xor[2]);
            } elsif(grep {$data[0] eq $_} ("30", "31")) {
                for(my $i = 0; $i < scalar(@data) - 1; $i++) {
                    $data[$i + 1] = sprintf("%02x", hex($data[$i + 1]) ^ $xor[$i % 4]);
                }
            }
            printf("%15.6f%s @data\n", $start, $ok == 1 ? ":" : "#");
        }
        @data = ();
        $last_frame = $framenum;
        $start = $time;
        if ($data == 0xa0 || ($showchan && $data == 0x80)) {
            $ok = 1;
        } elsif($telem && $data == 0x21) {
            $ok = 2;
            #push @data, sprintf("%02x", $data);
        } else {
            $ok = 0;
        }
    } elsif($ok == 1) {
        push @data, sprintf("%02x", $data);
    } elsif($ok == 2) {
        push @data, sprintf("%02x", $din);
    }
}
