#!/usr/bin/perl
# this script takes on stdin the ouput of the MikroElectronica GLCD FontCreator
# in 'mikroC' X-GLCD lib format
# The output is a font file compatible with the LCD routines

use warnings;
use strict;
use Getopt::Long;
use Data::Dumper;

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
    "europlus" => {
       range => [[0x20, 0x7e],
                 [0xa1, 0x101],
                 [0x0112, 0x0113],
                 [0x012a, 0x012b],
                 [0x0132, 0x0133],
                 [0x014c, 0x014d],
                 [0x0150, 0x0153],
                 [0x0160, 0x0161],
                 [0x016a, 0x016b],
                 [0x0170, 0x0171],
                 [0x0174, 0x0177],
                 [0x017d, 0x017e],
                 [0x1e80, 0x1e83]]
    },
);
my $template;
my %font;
my $max_height = 0;
my $array_w = 0;
my $array_h = 0;
my $name = "";
my $font_name;

main();

sub main
{
    my $range;
    my $export;
    my $import;
    my $old;
    GetOptions(
         "template=s"  => \$template,
         "export"      => \$export,
         "import=s"      => \$import,
         "old"         => \$old,
         "name=s"      => \$font_name);
    if(! $template) {
        $template = $templates{"ascii"};
    } else {
        $template = $templates{$template};
    }

    if($import) {
        import_font($import);
    } elsif($old) {
        process_file();
    } else {
        process_file2();
    }
    filter_chars();
    optimize();
    if($export) {
        export_font();
    } else {
        write_font();
    }
}
sub filter_chars {
    return if(! $template->{"range"} && ! $template->{"exclude"});
    my @chars = keys(%font);
    foreach my $char (@chars) {
        if ($template->{"exclude"}) {
            if(grep {$char == $_} @{ $template->{"exclude"} }) {
                $font{$char} = [0];
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
    my @ranges = @{ $template->{"range"} };
    for(my $i = 0; $i < scalar(@ranges); $i++) {
        while($ranges[$i][0] <= $ranges[$i][1]) {
            last if ( $font{$ranges[$i][0]});
            $ranges[$i][0]++;
        }
        if ($ranges[$i][0] > $ranges[$i][1]) {
            splice(@ranges, $i, 1);
            $i--;
            next;
        }
        while($ranges[$i][1] >= $ranges[$i][0]) {
            last if ( $font{$ranges[$i][1]});
            $ranges[$i][1]--;
        }
        if ($ranges[$i][0] > $ranges[$i][1]) {
            splice(@ranges, $i, 1);
            $i--;
        }
    }
    $template->{"range"} = [@ranges];
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
sub row_to_col
{
    my($width, $height, $data, $offset) = @_;
    my $bpc = get_bpc($height);
    my $bpr = get_bpc($width);
    my @rows;
    for(my $i = 0; $i < $height; $i++) {
        my $val = 0;
        for(my $j = 0; $j < $bpr; $j++) {
           $val |= $data->[$offset + $i*$bpr + $j] << (8 * $j);
        }
        push @rows, $val;
    }
    my @cols;
    for(my $i = 0; $i < $width; $i++) {
        my $val = 0;
        for(my $j = 0; $j < $height; $j++) {
            $val |= (($rows[$j] >> $i) & 0x01) << $j;
        }
        push @cols, $val;
    }
    return @cols;
}

sub process_file2
{
    my @data;
    while(<>) {
        if(/(\S+?)(\d+)x(\d+)\[\] =/) {
            ($name, $array_w, $array_h) = ($1, $2, $3);
        }
        next unless(/^\s*0x/);
        while(/0x(..)/g) {
            push @data, hex($1);
        }
    }
    my $start = ($data[3] << 8) + $data[2];
    my $end   = ($data[5] << 8) + $data[4];
    my $height = $data[6];
    foreach my $char ($start..$end) {
        my $idx = 8 + 4 * ($char - $start);
        my $width = $data[$idx];
        my $offset = ($data[3 + $idx] << 16) + ($data[2 + $idx] << 8) + $data[1 + $idx];
        $font{$char} = [$width, row_to_col($width, $height, \@data, $offset)];
    }
}

sub get_height
{
    my($char, $from) = @_;
    my @data = @$char;
    my $width = shift @data;
    my $height = 0;
    my $space;
    my $total = 9999;
    for(my $i = 0; $i < $width; $i++) {
        my $val = $data[$i];
        #find space at top
        for($space = 0; $space < $array_h; $space++) {
            if($from eq "bottom" && ($val & (1 << $space))) {
                last;
            }
            if($from eq "top" && ($val & (1 << ($array_h - 1 - $space)))) {
                last;
            }
        }
        $total = $space if ($space < $total);
    }
    return $array_h - $total;
}
sub max
{
    my($a, $b) = @_;
    return $a > $b ? $a : $b;
}

sub export_font
{
    mkdir $font_name;
    `echo $array_h > $font_name/height`;
    my $zero = get_height($font{65}, "top");
    print "$zero\n";
    foreach my $char (keys %font) {
        my $h1 = get_height($font{$char}, "top");
        my $h2 = get_height($font{$char}, "bottom");
        my $above = max(0, $h2 - ($array_h - $zero));
        my $below = max(0, $h1 - $zero);
        my ($width, @data)  = @{ $font{$char} };
        open my $fh, ">", "$font_name/${char}_${width}_${above}_${below}";
        my %tmp = (
            char => $char,
            width => $width,
            above =>  $above,
            below =>  $below,
            zero  =>  $zero,
            data => [@data],
        );
        print $fh Dumper(\%tmp);
        close $fh;
    }
}

sub font_add_space
{
    my($data, $rows) = @_;
    for(my $i = 1; $i < scalar(@$data); $i++) {
        $data->[$i] = $data->[$i] << $rows;
    }
}

sub import_font
{
    my($dir) = @_;
    my @files = glob("$dir/*");
    $array_h = 0;
    my $max = `cat $dir/height`;
    chomp $max;
    foreach my $file (@files) {
        next if($file =~/height/);
        our $VAR1;
        require $file;
        my $char = $VAR1->{char};
        my @d = ($VAR1->{width}, @{ $VAR1->{data} });
        font_add_space(\@d, max(0, $max - $VAR1->{zero}));
        $font{$char} = [@d];
        my $h = $max + $VAR1->{below};
        $array_h = $h if($h > $array_h);
    }
}

sub write_font
{
    my $max_width = 0;
    map {$max_width = $font{$_}[0] if($font{$_}[0] > $max_width)} keys(%font);
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
    printf " * first char  : 0x%02x\n", $chars[0];
    printf " * last char   : 0x%02x\n", $chars[-1];
    printf " * proportional: 1\n";
    printf " * bytes/col   : %d\n", $bpc;
    printf " */\n\n";
    printf "#ifndef FONTDECL\n";
    printf "const uint32_t ${range_var}[] = {\n";
    foreach my $r (@{ $template->{range} }) {
        printf "    0x%02x, 0x%02x,\n", $r->[0], $r->[1];
    }
    printf "    0x00\n};\n";
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
        printf "    // Char: 0x%02x(%s)\n", $c, ($c < 256) ? chr($c) : "";
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
