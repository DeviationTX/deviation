#!/usr/bin/env perl
use strict;
use warnings;
use integer;

use Getopt::Long;

my $target;
my $quiet;
GetOptions("target=s" => \$target, "quiet" => \$quiet);

my @dirs = grep {$_ !~ /common/ && (! $target || $_ =~ /$target/)} glob("filesystem/*");
my $max_line_length = 0;
my $max_bytes = 0;
my $max_count = 0;
my $error = 0;
foreach my $target_dir (@dirs) {
    my($target) = ($target_dir =~ /\/(.*)/);
    my @langfiles = glob("$target_dir/language/*");
    my $target_bytes = 0;
    my $target_count = 0;
    my $target_line_length = 0;
    foreach my $file (@langfiles) {
        my %hash;
        open my $fh, "<", $file || die("Couldn't read $file\n");
        my @lines = <$fh>;
        foreach (@lines) { chomp; }
        close $fh;
        shift @lines;
        my $bytes = 0;
        my $count = 0;
        my $line_length = 0;
        for (my $idx = 0; $idx < $#lines; $idx++) {
            if($lines[$idx] =~ /^:/) {
                my $hashval = fnv(substr($lines[$idx], 1));
                if($hash{$hashval}) {
                    print "Found hash collision between:\n    $hash{$hashval}\n    " . substr($lines[$idx], 1) . "\n";
                    $error = 1;
                }
                #printf "%6d: %s\n", $hashval, substr($lines[$idx], 1);
                $hash{$hashval} ||= substr($lines[$idx], 1);
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
$error |= ($allowed_line_length < $max_line_length || $allowed_count < $max_count || $allowed_bytes < $max_bytes);
exit($error);

sub fnv {
    my($str) = @_;
    my $orig_str = $str;
    $str =~ s/\\n/\n/g;
    my $hval = 0x811c9dc5;
    # FNV-1 hash each octet in the buffer
    my @s = split(//, $str);
    foreach (@s) {
	#/* multiply by the 32 bit FNV magic prime mod 2^32 */
	$hval += ($hval<<1) + ($hval<<4) + ($hval<<7) + ($hval<<8) + ($hval<<24);
        $hval &= 0xFFFFFFFF;

	#/* xor the bottom with the current octet */
	$hval ^= ord($_);
    }

    #/* fold to 16bits (don't do this if you want 32bits */
    $hval = 0xFFFF & (($hval>>16) ^ ($hval & (1<<16)-1));
    #/* return our new hash value */
    #printf "%5d: %s\n", $hval, $orig_str;
    return $hval;
}
