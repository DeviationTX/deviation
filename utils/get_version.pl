#!/usr/bin/env perl
use strict;
use warnings;
use FindBin;

my $target = shift (@ARGV);
my $fh;
my $tag;
my $version;
my $count = 0;
if (! -d "$FindBin::Bin/../.git") {
    #No hg found
    if (! -f "$FindBin::Bin/../.hg_archival.txt") {
        print "${target}-Unknown";
        exit 0;
    }
    open $fh, "$FindBin::Bin/../.hg_archival.txt";
    $count = 1;
#repo: 193697f59e5cf61947faf6102a5b3c786b1f50a3
#node: 6cec02426da64186f78bea51486e15659a6c16ad
#branch: default
#latesttag: v3.0.0
#latesttagdistance: 365
    while(<$fh>) {
        if(/^\s*(?:latest)?tag:\s+(.*\S)/) {
            $tag = $1;
        }
        elsif(/latesttagdistance:\s+(\d+)/) {
            $count = $1+1;
        }
        elsif(/node:\s+(.*\S)/) {
            $version = $1;
        }
    }
} else {
    if (! open $fh, "git describe --tags 2> /dev/null |") {
        print "${target}-Unknown";
        exit 0;
    }
    if (<$fh> =~ /^(.*)-(\d+)-([^-]*)$/) {
        ($tag, $count, $version) = ($1, $2, $3);
    }
}
if(! $version) {
    print "${target}-Unknown";
} else {
    $version = substr($version, 0, 7);
    if(! $tag) {
        print "${target}-$version";
    } elsif ($count == 1) {
        print "${target}-$tag";
    } else {
        print "${target}-$tag-" . substr($version, 0, 7);
    }
}
exit 0;

