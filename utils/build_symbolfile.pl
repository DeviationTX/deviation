#!/usr/bin/env perl
while(<>) {
	if(/^(\S+) g.....([FO])..* (\S+)/) {
            printf "$3 = 0x%08x;\n", hex($1) + ($2 eq "O" ? 0 : 1);
        } elsif(/^(\S+).*\*ABS\*\s+0+\s+_data_loadaddr/) {
            printf "_data_loadaddr = 0x%08x;\n", hex($1);
        }
}
