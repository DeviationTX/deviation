#!/usr/bin/env perl
use strict;
use warnings;

use Getopt::Long;
my $update;
my $lang;
my $target;
my $objdir;
my $count;
my @targets = ("320x240x16", "128x64x1");
my %targetmap = (
    devo8 => "320x240x16",
    devo12 => "480x272x16",
    devo10 => "128x64x1",
    devo7e => undef,
    devof7 => undef,
    x9d => "128x64x1",
);
my %alt_targets = (
    devo12 => ["devo8"],
);


$ENV{CROSS} ||= "";
GetOptions("update" => \$update, "language=s" => \$lang, "target=s" => \$target, "count" => \$count, "objdir=s" => \$objdir);
if($target && ! exists $targetmap{$target}) {
    my @t = keys(%targetmap);
    print "Target must be one of: @t\n";
    exit 1;
}
exit 0 if($target && ! $targetmap{$target});

my @lines = `/usr/bin/find . -name "*.[hc]" | grep -v libopencm3 | xargs xgettext -o - --omit-header -k --keyword=_tr --keyword=_tr_noop --no-wrap`;
my @files;
my $str = "";
my $idx = 0;
my %filemap;
#Read all strings and put into a hash that maps the containing file to the string
foreach (@lines) {
    if(/^\s*$/ || /^msgstr/) {
        if($str) {
            foreach my $f (@files) {
                $filemap{$f} = $str;
            }
        }
        @files = ();
        $str = "";
        $idx++;
    }
    if(/^#:\s+(.*\S)\s*$/) {
        foreach my $f (split /\s+/, $1) {
            push @files, "$f:$idx";
        }
    }
    next unless(/^(msgid|")/);
    if(/^msgid/) {
        $str = "";
    }
    s/^.*?"(.*)"\s*$/$1/;
    $str .= $_;
}
if($str) {
    foreach my $f (@files) {
        $filemap{$f} = $str;
    }
}
#Filter out any files that are not used by the specified target
#if($target) {
#    foreach my $f (keys %filemap) {
#        if(grep {$_ ne $targetmap{$target} && $f =~ /$_/} @targets) {
#            delete $filemap{$f};
#        }
#    }
#}
#build string list
my @out;
#Filter out any strings that do not appear in any obj files
if($objdir) {
    my %allstr;
    my @files = glob("$objdir/*.o");
    foreach my $file (@files) {
        #Parse all strings from the object files and add to the allstr hash
        my @od = `$ENV{CROSS}objdump -s $file`;
        my $str = "";
        my $state = 0;
        foreach(@od) {
            my $orig = $_;
            if(/section \.ro?data/) {
                $str = "";
                $state = 1;
                next;
            } elsif(/^Contents/ && ! /\.ro?data/) {
                $str = "";
                $state = 0;
            } elsif($state) {
                #Strip off everything but hex data
                s/^\s+\S+\s//;
                s/\s\s.*//;
                s/ //g;
                while(/(\S\S)/g) {
                    #Iterate over each ascii character
                    if($1 eq "00") {
                        #NULL termination
                        if($str) {
                            $str =~ s/\n/\\n/g;  #Convert <CR> to \n
                            $allstr{$str} = 1;
                            $str = "";
                        }
                    } else {
                        $str .= chr(hex($1));
                    }
                }
            }
        }
    }
    foreach (values %filemap) {
        if($allstr{$_}) {
            push @out, $_;
        } else {
            #print STDERR "Ignoring unused string '$_'\n";
        }
    }
} else {
    @out = values(%filemap);
}
#append template names
foreach (`head -n 1 filesystem/common/template/*.ini`) {
    chomp;
    if(/template=(.*?)\s*$/) {
        push @out, $1;
    }
}
my %uniq = map {$_ => undef} @out;
if($count) {
    printf "%d", scalar(keys(%uniq));
    exit 0;
}

if(! $update) {
    foreach (sort keys %uniq) {
        print ":$_\n";
    }
    exit 0;
}

@files = glob($lang ?
              "filesystem/common/language/lang*.$lang"
              : "filesystem/common/language/lang*");

foreach my $file (@files) {
    my %strings = %uniq;
    my %targetstr;
    my %unused;
    open my $fh, "<", $file;
    my $name = <$fh>;
    my(@lines) = <$fh>;
    for (my $line = 0; $line < $#lines; $line++) {
        $_ = $lines[$line];
        chomp;
        if(/^:(.*)/) {               #/^([<>]?):(.*)/)
            my($eng) = ($1);
            next if ($line < $#lines-2 && $lines[$line+1] =~ /^:./);
            my $next = $lines[++$line];
            chomp $next;
            #($next) = ($next =~ /^.(.*)/) if ($missing);
            if (! exists $uniq{$eng}) {
                $unused{$eng} = 1;
            }
            $strings{$eng} = $next;
        } elsif(/^\|(\S+?):(.*)/) {
            my($t, $eng) = ($1, $2);
            my $next = $lines[++$line];
            chomp $next;
            if (! exists $uniq{$eng}) {
                $unused{$eng} = 1;
            }
            $targetstr{$t}{$eng} = $next;
            $strings{$eng} ||= $next;
        }
        
    }
    if($target) {
        #if target is specified, we want to return a filtered list of strings
        my $outf = $file;
        $outf =~ s/common/$target/;
        open $fh, ">", $outf;
        print $fh $name;
        $targetstr{$target} ||= {};
        if($alt_targets{$target}) {
            #Hierarchically try to find best string
            foreach(@{ $alt_targets{$target} }) {
                $targetstr{$_} ||= {};
                %strings = (%strings, %{$targetstr{$_}});
            }
        }
        %strings = (%strings, %{$targetstr{$target}});
        foreach (sort keys %strings) {
            if(! $unused{$_} && defined($strings{$_})) {
                print $fh ":$_\n$strings{$_}\n";
            }
        }
        close $fh;
    } else {
        open $fh, ">", $file;
        print $fh $name;
        foreach my $str (sort (keys(%strings))) {
            if ($unused{$str}) {
                if (scalar(grep {defined $targetstr{$_}{$str}} keys(%targetmap)) != scalar(keys(%targetmap))) {
                    #print generic if not all targetshave a translation
                    print $fh "<:$str\n<$strings{$str}\n";
                }
                foreach my $key (sort keys %targetmap) {
                    if(defined $targetstr{$key}{$str}) {
                        print $fh "<|$key:$str\n<$targetstr{$key}{$str}\n";
                    }
                }
            } elsif(! defined($strings{$str})) {
                print $fh ">:$str\n";
            } else {
                if (scalar(grep {defined $targetstr{$_}{$str}} keys(%targetmap)) != scalar(keys(%targetmap))) {
                    #print generic if not all targetshave a translation
                    print $fh ":$str\n$strings{$str}\n";
                }
                foreach my $key (sort keys %targetmap) {
                    if(defined $targetstr{$key}{$str}) {
                        print $fh "|$key:$str\n$targetstr{$key}{$str}\n";
                    }
                }
            }
        }
        close $fh;
    }
}

# | xargs xgettext -o - --omit-header -k --keyword=_tr --keyword=_tr_noop --no-wrap | grep -v 'msgstr ""' | grep -v "^#" | grep -v "^$"

#: pages/dialogs.c:100
#msgid ""
#"Binding is in progress...\n"
#"Make sure model is on!\n"
#"\n"
#"Pressing OK will NOT cancel binding procedure\n"
#"but will allow full control of Tx."
#msgstr ""

