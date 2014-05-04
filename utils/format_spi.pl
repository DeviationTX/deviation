#!/usr/bin/perl
use strict;
use warnings;
use FindBin;
use Data::Dumper;
use Getopt::Long;

sub read_a7105 {
    my %cmd;
    my $h = "$FindBin::Bin/../src/protocol/iface_a7105.h";
    open my $fh, "<", $h or die "Couldn't read $h\n";
    while(<$fh>) {
        if(/^enum {/ .. /^};/) {
             if(/A7105_.._(\S+)\s*=\s*(0x..)/) {
                 $cmd{hex($2)} = $1;
             }
        }
    }
    return \%cmd;
}
sub read_nrf24l01 {
    my($long) = @_;
    my %cmd = (
        WR_MASK     => 0x20,
        CMD_MASK    => 0xC0,
        ADDR_MASK   => 0x1F
        );
    my $h = "$FindBin::Bin/../src/protocol/iface_nrf24l01.h";
    open my $fh, "<", $h or die "Couldn't read $h\n";
    while(<$fh>) {
        if(/^enum {/ .. /^};/) {
             if(/(NRF24L01_.._)(\S+)\s*=\s*(0x..)/) {
                 $cmd{hex($3)} = ($long ? $1 : "") . $2;
             }
        }
    }
    return \%cmd;
}

sub parse_spi {
    my($file) = @_;
    open my $fh, "<", $file or die "Couldn't read $file\n";
    my @data = ();
    my $basetime = -1;
    $_ = <$fh>;
    while(<$fh>) {
        s///;
        chomp;
        my($time, $idx, $mosi, $miso) = split(/,/, $_);
        next if ($idx eq "");
        $basetime = $time if($basetime == -1);
        $data[$idx] ||= [$time-$basetime];
        push @{$data[$idx]}, [hex($mosi), hex($miso)];
    }
    #[ [time, [mosi, miso], [mosi, miso], ...], [time, [mosi, miso], [mosi, miso], ...], ...
    return \@data;
}

sub main {
    my $long;
    GetOptions("long" => \$long);
    my $file = shift @ARGV;
    my $cmds = read_nrf24l01($long);
    my $data = parse_spi($file);
    my $cmdlen = 15;
    foreach (values %$cmds) {
        $cmdlen = length($_) if(length($_) > $cmdlen);
    }
    my $format = "%-10.6f %s %-${cmdlen}s      %-40s => %s\n";
    foreach my $d (@$data) {
        my @d = @$d;
        my($time) = shift @d;
        my @mosi;
        my @miso;
        foreach (@d) {
            push @mosi, sprintf("%02x", $_->[0]);
            push @miso, sprintf("%02x", $_->[1]);
        }
        my $cmdbyte = hex($mosi[0]);
        my $dir = "<";
        if ($cmdbyte & $cmds->{CMD_MASK}) {
            $dir = "=";
        } elsif ($cmdbyte & $cmds->{WR_MASK}) {
            $dir = ">";
        }
        my $cmdstr = $cmds->{$cmdbyte} || $cmds->{$cmdbyte & $cmds->{ADDR_MASK}} || "";
        printf $format, $time, $dir, $cmdstr, "@mosi", "@miso";
    }
}
main();
