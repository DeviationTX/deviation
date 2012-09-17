#!/usr/bin/perl
use strict;
use warnings;

use Data::Dumper;
my $userfunc = shift @ARGV;
my @data = <>;
my %tree;
my %seen;
sub parse {
    my($func, $ptr) = @_;
    my $cur = "";
    foreach my $line (@data) {
        if($line =~ /<(\S+)>:$/) {
            $cur = $1;
            next;
        }
        if($line =~ /<$func>$/) {
            $ptr->{$cur} = {};
        }
    }
}

sub recurse {
    my($func, $ptr) = @_;
    parse($func, $ptr);
    foreach my $k (keys %$ptr) {
        next if($seen{$k});
        $seen{$k} = 1;
        recurse($k, $ptr->{$k});
    }
}
$tree{$userfunc} = {};
recurse($userfunc, $tree{$userfunc});
print Dumper(\%tree);

