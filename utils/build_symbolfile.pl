#!/usr/bin/env perl
use strict;
use warnings;

use Getopt::Long;

my $exports_file = "";
my %exports;

GetOptions("exports=s" => \$exports_file);

open my $fh, "<", $exports_file;
while (<$fh>) {
	chomp;
	$exports{$_} = 1;
}
close($fh);

while(<>) {
	if(/^(\S+) g.....([FO])..* (\S+)/) {
            # Either in export table or it is a data
            if (exists($exports{$3}) || ($2 eq "O")) {
                printf "$3 = 0x%08x;\n", hex($1) + ($2 eq "O" ? 0 : 1);
            }
        } elsif(/^(\S+).*\*ABS\*\s+0+\s+_data_loadaddr/) {
            printf "_data_loadaddr = 0x%08x;\n", hex($1);
        }
}
