#!/usr/bin/perl
my $rom_start = 0;
my $rom_end = 0;
my $data_end = 0;
my $ram_start = 0;
my $ram_end = 0;
while(<>) {
    if(/^\.data\s+(0x\S+)/) {
        $ram_start = hex($1);
    } elsif(/(0x\S+)\s+_ebss = \./) {
        $ram_end = hex($1);
    } elsif(/(0x\S+)\s+_etext = \./) {
        $rom_end = hex($1);
    } elsif(/^\.text\s+(0x\S+)/) {
        $rom_start = hex($1);
    } elsif(/(0x\S+)\s+_edata = \./) {
        $data_end = hex($1);
    }
}
$rom_end += $data_end - $ram_start;
printf "ROM: 0x%08x - 0x%08x = %6.2fkB\n", $rom_start, $rom_end, ($rom_end - $rom_start) / 1024.0;
printf "RAM: 0x%08x - 0x%08x = %6.2fkB\n", $ram_start, $ram_end, ($ram_end - $ram_start) / 1024.0;
