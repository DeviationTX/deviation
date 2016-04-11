#!/usr/bin/env perl
use strict;
use warnings;
use FindBin;

my $target = shift (@ARGV);
my $fh;
my $tag;
my $version;
my $count = 0;
my $git_root = `git rev-parse --show-toplevel 2> /dev/null | tr -d '\n'`;
if (! $git_root || ! -d $git_root) {
    print "${target}-Unknown";
    exit 0;
} else {
    if (`git describe --tags 2> /dev/null` =~ /^(.*)-(\d+)-g([^-]*)$/) {
        ($tag, $count, $version) = ($1, $2, $3);
    } else {
        # This can fail if we have a shallow clone
        $version = `git rev-parse HEAD`;
        $tag = `git ls-remote --tags \`git config --get remote.origin.url\` | sort -Vk2 | tail -n 1 | sed -e 's/.*\\///'`;
        chomp($tag);
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

