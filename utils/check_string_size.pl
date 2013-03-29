#!/usr/bin/env perl
use strict;
use warnings;
use Getopt::Long;

my $target;
my $quiet;
GetOptions("target=s" => \$target, "quiet" => \$quiet);

my @dirs = grep {$_ !~ /common/ && (! $target || $_ =~ /$target/)} glob("filesystem/*");
my $max_line_length = 0;
my $max_bytes = 0;
my $max_count = 0;
foreach my $target_dir (@dirs) {
    my($target) = ($target_dir =~ /\/(.*)/);
    my @langfiles = glob("$target_dir/language/*");
    my $target_bytes = 0;
    my $target_count = 0;
    my $target_line_length = 0;
    foreach my $file (@langfiles) {
        open my $fh, "<", $file || die("Couldn't read $file\n");
        my @lines = <$fh>;
        close $fh;
        shift @lines;
        my $bytes = 0;
        my $count = 0;
        my $line_length = 0;
        for (my $idx = 0; $idx < $#lines; $idx++) {
            if($lines[$idx] =~ /^:/) {
                my $len = length($lines[$idx+1]) + 1; #Count the NULL too
                $bytes += $len;
                $line_length = $len if($len > $line_length);
                $count++;
                $idx++;
            }
        }
        printf("%-35s: %5d lines, %5d bytes, %4d bytes/line\n", $file, $count, $bytes, $line_length) if(! $quiet);
        $target_bytes = $bytes if($bytes > $target_bytes);
        $target_count = $count if($count > $target_count);
        $target_line_length = $line_length if($line_length > $target_line_length);
    }
    my @lines = `../utils/extract_strings.pl -target $target`;
    my $count = scalar(@lines);
    printf("%-35s: %5d lines\n", $target, $count) if(! $quiet);
    $target_count = $count if($count > $target_count);
    printf("%-35s: %5d lines, %5d bytes, %4d bytes/line\n", $target_dir, $target_count, $target_bytes, $target_line_length) if(! $quiet);
    $max_bytes = $target_bytes if($target_bytes > $max_bytes);
    $max_count = $target_count if($target_count > $max_count);
    $max_line_length = $target_line_length if($target_line_length > $max_line_length);
}
printf("%-35s: %5d lines, %5d bytes, %4d bytes/line\n", "Total", $max_count, $max_bytes, $max_line_length) if(! $quiet);
open my $fh, "config/language.c" || die("Couldn't parse: config/language.c");
my $allowed_line_length = 0;
my $allowed_count = 0;
my $allowed_bytes = 0;
while(<$fh>) {
    if(/char strings\[(\d+)\]/) {
        $allowed_bytes = $1;
    } elsif(/MAX_STRINGS\s+(\d+)/) {
        $allowed_count = $1;
    } elsif(/MAX_LINE\s+(\d+)/) {
        $allowed_line_length = $1;
    }
}
printf("%-35s: %5d lines, %5d bytes, %4d bytes/line\n", "Allocated", $allowed_count, $allowed_bytes, $allowed_line_length) if(! $quiet);
my $ok = ($allowed_line_length >= $max_line_length && $allowed_count >= $max_count && $allowed_bytes >= $max_bytes);
exit(! $ok);
