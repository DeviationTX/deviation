#!/usr/bin/perl
use strict;
use warnings;
use Getopt::Long;
my $mfgid = "";
my @xor = (0, 0, 0, 0);
my $showchan = 0;
GetOptions("mfgid=s" => \$mfgid, "chan" => \$showchan);
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
    my($time, $framenum, $data) = split(/,/, $_);
    next unless($time && $framenum && $data);
    $data = hex($data);
    if($framenum != $last_frame) {
        if(@data) {
            if(grep {$data[0] eq $_} ("87", "8b", "8c")) {
                for(my $i = 0; $i < 15; $i++) {
                    $data[$i + 1] = sprintf("%02x", hex($data[$i + 1]) ^ $xor[$i % 4]);
                }
            } elsif($data[0] eq "8a") {
                $data[13] = sprintf("%02x", hex($data[13]) ^ $xor[0]);
                $data[14] = sprintf("%02x", hex($data[14]) ^ $xor[1]);
                $data[15] = sprintf("%02x", hex($data[15]) ^ $xor[2]);
            }
            printf("%15s: @data\n", $start);
        }
        @data = ();
        $last_frame = $framenum;
        $start = $time;
        $ok = ($data == 0xa0 || ($showchan && $data == 0x80)); #include channel change
    } elsif($ok) {
        push @data, sprintf("%02x", $data);
    }
}
