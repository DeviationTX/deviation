#!/usr/bin/env perl

use Data::Dumper;

my @data = <>;
my @backtrace;
my $target;
for($i = 0; $i <= $#data; $i++) {
    if($data[$i] =~ /\[.* fault\]/) {
        ($target) = ($data[$i+1] =~ /^([^-]+)/);
    }
    if($data[$i] =~ /Backtrace:/) {
        $i++;
        for(; $i <= $#data; $i++) {
            if($data[$i] =~ /: (\S+)/) {
                push @backtrace, hex("0x".$1)-1;
            }
        }
    }
}
open my $fh, "<", "$target.list" or die "Couldn't open $target.list\n";
@data = <$fh>;
close $fh;
my @lookup;
my $i = 0;
foreach (@data) {
    if(/^([0-9a-f]+) *<(.*)>/) {
        push @lookup, [hex("0x".$1), $2, $i];
    }
    $i++;
}
push @lookup, [0xffffffff, "EOF", $i];
foreach my $addr (@backtrace) {
    for($i = 0; $i <= $#lookup; $i++) {
        if($addr >= $lookup[$i][0] && $addr < $lookup[$i+1][0]) {
            my $func = $lookup[$i][1];
            if($addr == $lookup[$i][0]) {
                printf("0x%08x => %s\n", $addr, $func);
                last;
            }
            my $last = $lookup[$i][2];
            for(my $j = $lookup[$i][2]; $j < $lookup[$i+1][2]; $j++) {
                if($data[$j] =~ /^ *([0-9a-f]+):/) {
                    my $ptr = hex("0x".$1);
                    if($ptr >= $addr) {
                        printf("0x%08x => $func\n\t$data[$last]", $addr);
                        last;
                    }
                    $last = $j;
                }
             }
             if($j == $lookup[$i+1][2]) {
                 printf("0x%08x => ???\n", $addr);
            }
            last;
        }
    }
}
