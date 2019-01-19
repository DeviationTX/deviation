#!/usr/bin/perl
use strict;
use warnings;

my @rules = qw(
    -whitespace/line_length
    -legal
    -build/header_guard
    -build/include
    -build/include_subdir
    -readability/casting
);

my @exclude = qw(libopencm3/ FatFs/);

use Getopt::Long;

die "Please install cpplint via 'pip install cpplint' or equivalent\n" if (! `which cpplint`);

my $debug = 0;
sub main {
    my $diff;
    GetOptions("diff" => \$diff, "debug" => \$debug);
    my @paths = @ARGV;
    my $changed;
    if ($diff) {
        $changed = get_changed_lines();
    }
    @paths = filter_paths(\@paths, $changed);
    run_lint(\@paths, $changed)
}

sub filter_paths {
    my($paths, $changed) = @_;
    my @ret;
    if (! $changed) {
        if (! @$paths) {
            return (".");
        }
        return @$paths;
    }
    if (! @$paths) {
        return sort keys(%$changed);
    }
    my $cmd = "find @$paths -type f";
    $cmd .= " | grep -v -E '(" . join("|", @exclude) . ")'" if(@exclude);
    open my $fh, "-|", $cmd or die "Can't run $cmd\n";
    while(<$fh>) {
        chomp;
        s/^\.\///;
        push @ret, $_ if ($changed->{$_});
    }
    return @ret;
}

sub get_changed_lines {
    my %changed;
    my $cmd = "git diff --relative --name-only --diff-filter AM";
    my $sha1 = "000000000";
    if ($ENV{TRAVIS_BRANCH}) {
        $cmd .= " HEAD..$ENV{TRAVIS_BRANCH}";
        $sha1 = `git rev-parse --short=9 $ENV{TRAVIS_BRANCH}`;
        chomp($sha1);
    }
    my @files = split(/\n/, `$cmd`);
    foreach my $file (@files) {
        $changed{$file} = {};
        open my $fh, "-|", "git blame $file" or die "Failed to run: git blame $file\n";
        while(<$fh>) {
            next unless(/^$sha1\s[^\(]*\([^\)]*\s(\d+)\)/);
            $changed{$file}{$1} = 1;
        }
        my @used = sort({$a <=> $b} keys(%{ $changed{$file} }));
        print "$file: @used\n" if($debug);
    }
    return \%changed;
}

sub run_lint {
    my($paths, $changed) = @_;
    my $cmd = "find @$paths -name \"*.[ch]\"";
    $cmd .= " | grep -v -E '(" . join("|", @exclude) . ")'" if(@exclude);
    $cmd .= " | xargs cpplint --extensions=c,h --filter=" . join(",", @rules) . " 2>&1";
    print "$cmd\n" if($debug);
    my %errors;
    my %violations;
    open my $fh, "-|", $cmd or die "Error: Can't run $cmd\n";
    while(<$fh>) {
        if (/(\S+):(\d+):\s.*\[(\S+)\]\s\[\d\]$/) {
            next if ($changed && ! $changed->{$1}{$2});
            print;
            $errors{$1}++;
            $violations{$3}++;
        }
    }

    print "\nSummary\n-------\n";
    for my $err (sort keys %violations) {
        printf "%-30s: %d\n", $err, $violations{$err};
    }
}

main();
