#!/usr/bin/env perl
use Getopt::Long;

my $skip = 31;
my $width = 14;
GetOptions("skip=i" => \$skip, "width=i" => \$width);
my $bytes = int(1 + ($width - 1) / 8);
my $line = 0;
while(<>) {
    if($line++ < $skip) {
        print;
        next;
    }
    chomp;
    my $comment = "";
    if(/(\/\/.*)/) {
        $comment = $1;
        s/\/\/.*//;
    }
    my @vals = ();
    while(/(0x..)/g) {
        push @vals, hex($1);
    }
    if(! @vals) {
        print "$_$comment\n";
        next;
    }
    my $cols = int(scalar(@vals) / $bytes);
    my @newvals;
    for(my $i = 0; $i < $cols; $i++) {
        push @newvals, sprintf("0x%02x", $vals[$i]);
        for(my $j = 1; $j < $bytes; $j++) {
            my $shift = ($j == $bytes - 1) ? ($j + 1)  * 8 - $width : 0;
            push @newvals, sprintf("0x%02x", $vals[$cols*$j + $i] >> $shift);
        }
    }
    print "    " . join(", ", @newvals) . ", $comment\n";
}
