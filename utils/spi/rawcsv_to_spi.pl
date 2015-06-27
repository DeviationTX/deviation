#!/usr/bin/env perl
use strict;
use warnings;
use Getopt::Long;

# Utility to convert raw CSV dumps from Logic(tm) to SPI

my($mosi, $miso, $clk, $enable) = (4, 3, 2, 1);
my $sample = 8000000;
GetOptions("mosi=i" => \$mosi, "miso=i" => \$miso, "clk=i" => \$clk, "enable=i" =>\$enable, "sample=i", \$sample);
my $last_en = 1;
my $last_clk;
my $packet = 0;
my $in = 0;
my $out = 0;
my $c = 0;
my $last_c = 0;
my $time = 0;
my $tick = 0;

print "Time [s],Packet ID,MOSI,MISO\n";
my $last = "";
$_ = <>;
while(<>) {
    $tick++;
    if($tick % 100000 == 0) {
        #print STDERR "$tick\n";
    }
    next if($last eq $_);
    $last = $_;
    s/\r+//;
    chomp;
    my @data = split(/\s*,\s*/, $_);
    if($enable && $data[$enable]) {
        if ($last_en) {
            if ($c == 4) {
                $out <<= 4;
                $in <<= 4;
            }
            if ($c == 4 || $c == 8) {
                printf("%11.9f,$packet,0x%02X,0x%02X\n", $sample ? $tick * 1.0 / $sample : $data[0], $out, $in);
            }
            $last_en = 0;
            $in = 0;
            $out = 0;
            $c = 0;
            $last_clk = 0;
        }
        next;
    }
    if(($enable && ! $last_en) || (!$enable && $data[0] - $tick > 20)) {
        $packet++;
        $last_en = 1;
            $in = 0;
            $out = 0;
            $c = 0;
            $last_clk = $data[$clk];
            next;
    }
    if($data[$clk] && ! $last_clk) {
        $c++;
        $in = ($in << 1) | $data[$miso];
        $out = ($out << 1) | $data[$mosi];
        # print "$_ ($c: $in)\n";
    }
    $last_clk = $data[$clk];
    $tick = $data[0];
    if($c == 8) {
        printf("%11.9f,$packet,0x%02X,0x%02X\n", $sample ? $tick * 1.0 / $sample : $data[0], $out, $in);
        $c = 0;
        $in = 0;
        $out = 0;
    }
}
