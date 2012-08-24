#!/usr/bin/perl
# this script takes on stdin the ouput of the MikroElectronica GLCD FontCreator
# in 'mikroC' X-GLCD lib format
# The output is a font file compatible with the LCD routines

use warnings;
use strict;
use Getopt::Long;

my %templates = (
    "none" => {},
    "number" => {
       range => [[0x25, 0x39]],
       exclude => [38, 39, 40, 41, 42, 44],
    },
    "ascii" => {
       range => [[0x20, 0x7e]],
    },
    "asciicaps" => {
       range => [[0x20, 0x5a]],
    },
    "european" => {
       range => [[0x20, 0x7e], [0xa1, 0xff]],
    },
);
my $template;
my %font;
my $max_width = 0;
my $max_height = 0;
my $array_w = 0;
my $array_h = 0;
my $name = "";
my $font_name;

main();

sub main
{
    my $font_pt = 0;
    my $range;
    GetOptions("points|pt=i" => \$font_pt,
               "template=s"  => \$template,
               "name=s"      => \$font_name);
    if(! $template) {
        $template = $templates{"ascii"};
    } else {
        $template = $templates{$template};
    }

    process_file();
    filter_chars();
    optimize();
    write_font();
}
sub filter_chars {
    return if(! $template->{"range"} && ! $template->{"exclude"});
    my @chars = keys(%font);
    foreach my $char (@chars) {
        if ($template->{"exclude"}) {
            if(grep {$char == $_} @{ $template->{"exclude"} }) {
                $font{$char} = undef;
                next;
            }
        }
        if ($template->{"range"}) {
            my $ok = 0;
            foreach my $range (@{ $template->{"range"} }) {
                if ($char >= $range->[0] && $char <= $range->[1]) {
                    $ok = 1;
                    last;
                }
            }
            if(! $ok) {
                delete $font{$char};
            }
        }
    }
}

sub optimize
{
    my($top_space) = $array_h;
    my($bottom_space) = $array_h;
    my($left_space) = 9999;

    foreach my $char (keys %font) {
        my $data= $font{$char};
        my $width = $data->[0];
        my $space;
        #Iterate over each column
        for(my $i = 1; $i <= $width; $i++) {
            my $val = $data->[$i];
            #find space at top
            for($space = 0; $space < $array_h; $space++) {
                if($val & (1 << $space)) {
                    last;
                }
            }
            if($space < $top_space) {
                #printf "Reducing top space to $space in '%c'(%02x)\n", $char, $char;
                $top_space = $space;
            }
            #find space at bottom
            for($space = 0; $space < $array_h; $space++) {
                if($val & (1 << ($array_h - 1 - $space))) {
                    last;
                }
            }
            if($space < $bottom_space) {
                #printf "Reducing bottom space to $space in '%c'(%02x)\n", $char, $char;
                $bottom_space = $space;
            }
        }
        #find space at left
        for(my $space = 0; $space < $width; $space++) {
            if($data->[$space+1] != 0) {
                last;
            }
        }
        if($space < $left_space) {
            #printf "Reducing left space to $space in '%c'(%02x)\n", $char, $char;
            $left_space = $space;
        }
    }
    #printf "Left space: $left_space\nTop space: $top_space\nBottom space: $bottom_space\n";
    foreach my $char (keys %font) {
        my $data= $font{$char};
        #remove left_space
        while($data->[0] > 4 && $data->[1] == 0) {
            splice(@$data, 1, 1);
            $data->[0]--;
        }
        for(my $c = 1; $c <= $data->[0]; $c++) {
            $data->[$c] = $data->[$c] >> $top_space;
        }
    }
    $array_h -= $top_space + $bottom_space;
    $max_width -= $left_space;
} 
sub get_bpc
{
    my($height)= @_;
    return int(($height - 1) / 8) + 1;
}

sub process_file
{
    my $bpc = 0;
    my $char = -1;
    while(<>) {
        if(/(\S+?)(\d+)x(\d+)\[\] =/) {
            ($name, $array_w, $array_h) = ($1, $2, $3);
            $bpc = get_bpc($array_h);
        }
        next unless(/\/\/ Code for char/);
        if(/Code for char (.)$/) {
            $char = ord($1);
        } else {
            $char++;
        }
        my @data;
        while(/0x(\S\S)/g) {
            push @data, hex($1);
        }
        my $width = shift(@data);
        
        if ($width > $max_width) {
            $max_width = $width;
        }

        #convert each column into an integer
        my @data_int = ();
        for(my $c = 0; $c < $width; $c++) {
            my $val = 0;
            for(my $b = 0; $b < $bpc; $b++) {
                $val |= $data[$c * $bpc + $b] << (8 * $b);
            }
            push @data_int, $val;
        }
        $font{$char} = [$width, @data_int];
    }
}

sub write_font
{
    my @chars = sort {$a <=> $b} keys(%font);
    my $bpc = int(($array_h - 1) / 8) + 1;
    my $font_var = "Font_${name}_${array_h}";
    my $range_var = "CharRange_${name}_${array_h}";
    printf "/*\n";
    printf " * Converted from X-GLCD by convert_xglcd_to_font.pl\n";
    printf " * " . scalar(localtime()) .  "\n";
    printf " *\n";
    printf " * width       : %d\n", $max_width;
    printf " * height      : %d\n", $array_h;
    printf " * first char  : 0x%02x (%s)\n", $chars[0], chr($chars[0]);
    printf " * last char   : 0x%02x (%s)\n", $chars[-1], chr($chars[-1]);
    printf " * proportional: 1\n";
    printf " * bytes/col   : %d\n", $bpc;
    printf " */\n\n";
    printf "#ifndef FONTDECL\n";
    printf "const uint32_t ${range_var}[] = {\n    ";
    foreach my $r (@{ $template->{range} }) {
        printf "0x%02x, 0x%02x, ", $r->[0], $r->[1];
    }
    printf "0x00\n};\n";
    printf "const uint8_t ${font_var}[] = {\n";
    my $count = 0;
    printf "    //font widths\n   ";
    foreach my $c (@chars) {
        printf " 0x%02x,", $font{$c}[0];
        if(++$count == 10) {
            printf"\n   ";
            $count = 0;
        }
    }
    printf "\n    // font data\n";
    foreach my $c (@chars) {
        printf "    // Char: 0x%02x(%s)\n", $c, chr($c);
        my @cols = @{ $font{$c}};
        my $width = shift @cols;
        if ($bpc > 1) {
            for (my $i = 0; $i < $width; $i++) {
                my @vals;
                my $val = $cols[$i];
                for(my $j = 0; $j < $bpc; $j++) {
                    push @vals, $val & 0xff;
                    $val = $val >> 8;
                }
                printf "    " . join ", ", map {sprintf "0x%02x", $_} @vals;
                printf ",\n";
            }
        } else {
            printf "    %s,\n", join ", ", map {sprintf "0x%02x", $_} @cols;
        }
    }
    printf "\n};";
    printf "\n#else //FONTDECL\n";
    printf "FONTDECL(0x80 | $max_width, $array_h, $range_var, $font_var, \"$font_name\")\n";
    printf "#endif //FONTDECL\n";
}
