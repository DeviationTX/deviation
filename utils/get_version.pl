#!/usr/bin/perl
use strict;
use warnings;

my $target = shift (@ARGV);
my $fh;
if (! open $fh, "hg log -f --template 'Version: {node} Tags: {tags}\n' 2> /dev/null |") {
    print "${target}-Unknown";
    exit 0;
}
my $count = 0;
my $tag;
my $version;
while(<$fh>) {
    chomp;
    if(! /Version:\s+(\S+)\s+Tags:\s+(.*)/) {
        next;
    }
    $version ||= $1;
    if($2 && $2 ne "tip") {
        $tag = $2;
        last;
    }
    $count++;
}
if(! $version) {
    print "${target}-Unknown";
} elsif(! $tag) {
    print "${target}-$version";
} elsif ($count == 1) {
    print "${target}-$tag";
} else {
    print "${target}-$tag-" . substr($version, 0, 7);
}
exit 0;

