#!/usr/bin/env perl
use warnings;
use strict;

use Data::Dumper;
use Getopt::Long;

my $debug = 0;
my $ascent = 0;
my $descent = 0;
my $maxsize = 0;
my $minspace = 0;
main();
sub main {
    my $out = "bdf_font";
    my $mode = "export";
    GetOptions("out=s" => \$out,
               "debug=i" => \$debug,
               "mode=s" => \$mode,
               "minspace=i" => \$minspace, 
               "maxsize=i" => \$maxsize);
    my $char = read_bdf(shift @ARGV);

    $out =~ s/\.fon$//;
    if($mode =~ /analyze/) {
        analyze_chars($char);
    }
    if($mode =~ /export/) {
        export_chars($char, $out);
    }
    if($mode =~ /bin/) {
        build_bin($char, $out)
    }
}

sub read_bdf {
    my($bdf_file)= @_;
    open my $fh, "<", $bdf_file;
    my $current_char = 0;
    my %char;
    while(<$fh>) {
        chomp;
        if(/FONT_ASCENT\s+(\d+)/) {
            ${ascent} = $1;
        }
        if(/FONT_DESCENT\s+(\d+)/) {
            ${descent} = $1;
        }
        if(/STARTCHAR\s+uni(\S\S\S\S)\s*$/) {
            $current_char = hex($1);
        } elsif(/ENCODING\s+(\d+)/) {
            $current_char = $1;
        } elsif(/BBX\s+(\S+) (\S+) (\S+) (\S+)/) {
            $char{$current_char}{bbox} = [$1, $2, $3, $4];
        } elsif(/BITMAP/) {
            my $bytes_per_row = int(($char{$current_char}{bbox}[0] +7) / 8);
            my @data;
            $_ = <$fh>;
            while(! /ENDCHAR/) {
                chomp;
                s/\s+//g;
                my $val = 0;
                my $bytes = 0;
                while(/(..)/g) {
                    $val = ($val << 8) | hex($1);
                    $bytes++;
                    last if($bytes == $bytes_per_row);
                }
                push @data, $val;
                $_ = <$fh>;
            }
            $char{$current_char}{bmp} = [@data];
        }
    }
    return \%char;
}

sub analyze_chars {
    my($char) = @_;
    foreach my $c (sort {$a <=> $b} keys %$char) {
        my $maxh = $char->{$c}{bbox}[1] + $char->{$c}{bbox}[3];
        my $minh = $maxh - $char->{$c}{bbox}[1];
        my $height;
        if ($maxh > $ascent || $minh < - $descent) {
            $height = $ascent + $descent;
            $height += $maxh - $ascent if ($maxh > $ascent);
            $height -= $minh + $descent if ($minh < -$descent);
        } else {
            $height = $char->{$c}{bbox}[1];
        }
        my $width  = $char->{$c}{bbox}[0];
        printf "%04x : %d x %d / %d ($maxh, $minh, $ascent, $descent)\n", $c, $width, $height, $char->{$c}{bbox}[1];
    }
}

sub build_font_hash {
    my($char) = @_;
    my $max_height = 0;
    my %font;
    foreach my $c (sort {$a <=> $b} keys %$char) {
        if ($c == $debug) {
            print "here\n";
        }
        my $width = $char->{$c}{bbox}[0] + $char->{$c}{bbox}[2];
        my $bpc = 8*int(($char->{$c}{bbox}[0] + 7) / 8);
        if ($char->{$c}{bbox}[3] > 0) {
            for my $i (1 .. $char->{$c}{bbox}[3]) {
                push @{$char->{$c}{bmp}}, 0;
            }
        }
        my @transpose;
        if ($char->{$c}{bbox}[2] > 0) {
            for my $i (1 .. $char->{$c}{bbox}[2]) {
                push @transpose, 0;
            }
        }
        foreach my $x (1 .. $char->{$c}{bbox}[0]) {
            my $val = 0;
            my $y;
            for ($y = $#{ $char->{$c}{bmp}}; $y >= 0; $y--) {
                $val = ($val << 1) | ($char->{$c}{bmp}[$y] & (1 << ($bpc-$x)) ? 1 : 0);
            }
            push @transpose, $val;
        }
        my $below = $char->{$c}{bbox}[3] < 0 ? - $char->{$c}{bbox}[3] : 0;
        my $above = $char->{$c}{bbox}[1] + $char->{$c}{bbox}[3];
        if($above + $below > $max_height) {
            if ($maxsize && $above + $below > $maxsize && $maxsize > $max_height) {
                $max_height = $maxsize;
            } else {
                $max_height = $above + $below;
            }
        }
        my $zero = $char->{$c}{bbox}[1] + $char->{$c}{bbox}[3];
        $font{$c} = {
            below => $below,
            above => $above,
            width => $width,
            char => $c,
            data => [@transpose],
            zero => $zero,
        };
    }
    return($max_height, \%font);
}
sub align_chars {
    my($font) = @_;
    my $max_zero = $ascent;
    #Align to baseline
    foreach my $c (keys %$font) {
        if($font->{$c}{zero} < $max_zero) {
            foreach my $col (@{ $font->{$c}{data} }) {
                $col <<= $max_zero - $font->{$c}{zero};
            }
            $font->{$c}{zero} = $max_zero;
        }
    }
}

sub export_chars {
    my($char, $dir) = @_;
    mkdir $dir;
    my($max_height, $font) = build_font_hash($char);
    foreach my $c (keys %$font) {
        my $width = $font->{$c}{width};
        my $above = $font->{$c}{above};
        my $below = $font->{$c}{below};
        open my $oFH, ">", "$dir/${c}_${width}_${above}_${below}";
        print $oFH Dumper($font->{$c});
        close $oFH;
    }
    system("echo $max_height > $dir/height");
}

sub build_font_range {
    my($chars) = @_;
    my @ranges = ();
    my $start = 0;
    my $end = 0;
    foreach my $char (sort {$a <=> $b} keys %$chars) {
        if($char != $end +1) {
            if ($end != 0) {
                if ($char - $end <= $minspace) {
                    foreach my $i ($end+1 .. $char-1) {
                        $chars->{$i} = {width => 0};
                    }
                    $end = $char;
                    next;
                }
                push @ranges, [$start, $end];
            }
            $start = $char;
            $end = $char;
        } else {
            $end++;
        }
    }
    push @ranges, [$start, $end];
    printf "Found %d ranges\n", scalar(@ranges);
    return @ranges;
}

sub build_bin {
    my($char, $file) = @_;
    my($max_height, $font) = build_font_hash($char);
    align_chars($font);
    my @range = build_font_range($font);
    open my $fh, ">", "$file.fon";

    my $pos = 5;
    my $bpc = int(($max_height + 7) / 8);
    my @chars = sort {$a <=> $b} keys(%$font);

    print $fh chr($max_height);
    foreach my $r (@range) {
        print $fh chr(($r->[0] >> 0) & 0xff); #range start
        print $fh chr(($r->[0] >> 8) & 0xff);
        print $fh chr(($r->[1] >> 0) & 0xff);
        print $fh chr(($r->[1] >> 8) & 0xff); #range end
        $pos += 4;
    }
    print $fh chr(0) . chr(0) . chr(0) . chr(0); #End of range table

    #Char offset
    $pos += (1 + scalar(@chars)) * 3; # length of offset table
    foreach my $c (@chars) {
        my $width = $font->{$c}{width};
        print $fh chr(($pos >> 0) & 0xff); #position of character (relative to top of file)
        print $fh chr(($pos >> 8) & 0xff);
        print $fh chr(($pos >> 16) & 0xff);
        $pos += $width * $bpc;
    }
    print $fh chr(($pos >> 0) & 0xff); #defines the width of the last character
    print $fh chr(($pos >> 8) & 0xff);
    print $fh chr(($pos >> 16) & 0xff);

    foreach my $c (@chars) {
        my $width = $font->{$c}{width};
        next unless($width);
        my @cols = @{ $font->{$c}{data}};
        if ($bpc > 1) {
            for (my $i = 0; $i < $width; $i++) {
                my @vals;
                my $val = $cols[$i];
                for(my $j = 0; $j < $bpc; $j++) {
                    push @vals, $val & 0xff;
                    $val = $val >> 8;
                }
                map { print $fh chr($_) } @vals;
            }
        } else {
            map { print $fh chr($_) } @cols;
        }
    }
    close $fh;
}
