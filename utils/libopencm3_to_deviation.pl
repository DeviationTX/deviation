#!/usr/bin/env perl

# This script will modify an existing libopencm3 clone so that
# it has only what is needed for deviation
# Usage:
#    libopencm3_to_deviation.pl -gitdir ~/git/libopencm3/ -outdir ./libopencm3/

use strict;
use warnings;
use Getopt::Long;
my $gitdir;
my $outdir;
GetOptions("gitdir=s" => \$gitdir, "outdir=s" => \$outdir);
if(! -d $gitdir) {
    die "Could not locate git src dir: " . ($gitdir || "") . "\n";
}
if(! -d $outdir) {
    die "Could not locate output src dir: " . ($outdir || "") . "\n";
}
my $gitver = `cd $gitdir && git log --pretty=format:%H%n HEAD^.. | tr -d '\n'`;
print "Removing old libopencm3\n";
system("rm -rf $outdir");
print "Copying libopencm3...\n";
system("cp -prf $gitdir/ $outdir/");
print "Removing git repository\n";
system("rm -rf $outdir/.git $outdir/.gitignore");
print "libopencm3 version: $gitver\n";
system("echo $gitver > $outdir/GIT_VERSION");
print "Generating headers\n";
system("cd $outdir;  make generatedheaders V=1");
print "Removing header templates\n";
system("find $outdir -name 'irq.yaml' | xargs rm -f");
