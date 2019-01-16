#!/usr/bin/env perl
use strict;
use warnings;

use Getopt::Long;

my $update;
my $lang;
my $fs;
my $target_list;
my $objdir;
my $elffile;
my $count;
# The following are legal alternatives to the default string
my @targets = ("devo8", "devo10", "devo12");

sub fnv_16 {
    my ($input, $init_value) = @_;

    $init_value = 0x811c9dc5 unless (defined $init_value);
    my $fnv_32_prime = 0x01000193;

    my $hval = $init_value;

    foreach my $x (unpack ('C*', $input)) {
        $hval = ($hval * $fnv_32_prime) & 0xffffffff;
        $hval = ($hval ^ $x) & 0xffffffff;
    }

    return ($hval >> 16) ^ ($hval & 0xffff);
}

$ENV{CROSS} ||= "";
GetOptions("update" => \$update, "language=s" => \$lang, "fs=s" => \$fs,
    "targets=s" => \$target_list, "count" => \$count,
    "objdir=s" => \$objdir, "elffile=s" => \$elffile);
my @requested_targets = ();
@requested_targets = split(/,/, $target_list) if($target_list);
if($fs || @requested_targets) {
    if (! @requested_targets || ! $fs) {
        print "ERROR: Must specify both -fs and -targets\n";
        exit 1;
    }
    my $ok = 1;
    foreach my $t (@requested_targets) {
        if(! grep {$_ eq $t} @targets) {
            print "ERROR: Target '$t' is not in list (@targets)\n";
            $ok = 0;
        }
    }
    exit 1 if(! $ok);
}

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
@files = ();
#Filter out any strings that do not appear in any obj files
if ($objdir) {
    print $objdir;
    push @files, glob("$objdir/*.o");
}

if ($elffile) {
    print "add $elffile";
    push @files, $elffile;
}

foreach my $file (@files) {
    my %allstr;
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
    foreach (values %filemap) {
        if($allstr{$_}) {
            push @out, $_;
        } else {
            #print STDERR "Ignoring unused string '$_'\n";
        }
    }
}

if (!$objdir && !$elffile) {
    @out = values(%filemap);
}
#append template names
foreach (`head -n 1 fs/common/template/*.ini`) {
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
              "fs/language/lang*.$lang"
              : "fs/language/lang*");

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
    if($fs) {
        #if target is specified, we want to return a filtered list of strings
        my $outf = $file;
        my %hashvalues;
        $outf =~ s/fs/filesystem\/$fs/;
        open $fh, ">", $outf or die "ERROR: Can't write $outf\n";
        print $fh $name;
        foreach (@requested_targets) {
            #Hierarchically try to find best string
            $targetstr{$_} ||= {};
            %strings = (%strings, %{$targetstr{$_}});
        }
        foreach (sort keys %strings) {
            if(! $unused{$_} && defined($strings{$_})) {
                if ($_ ne $strings{$_}) {
                    my $hash = fnv_16($_);
                    my $value = $strings{$_};
                    if (defined($hashvalues{$hash})) {
                        printf("Warning: Conflict hash detected:\n%s\n%s\n",
                            $hashvalues{$hash}, $value);
                        exit 1;
                    }
                    $hashvalues{$hash} = $value;
                }
            }
        }

        foreach(sort{$a <=> $b} keys %hashvalues)
        {
            print $fh pack("CC", $_ & 0xFF, $_ >> 8);
            print $fh "$hashvalues{$_}\n";
        }
        close $fh;
    } else {
        open $fh, ">", $file;
        print $fh $name;
        foreach my $str (sort (keys(%strings))) {
            if ($unused{$str}) {
                if (scalar(grep {defined $targetstr{$_}{$str}} @targets) != scalar(@targets)) {
                    #print generic if not all targetshave a translation
                    print $fh "<:$str\n<$strings{$str}\n";
                }
                foreach my $key (sort @targets) {
                    if(defined $targetstr{$key}{$str}) {
                        print $fh "<|$key:$str\n<$targetstr{$key}{$str}\n";
                    }
                }
            } elsif(! defined($strings{$str})) {
                print $fh ">:$str\n";
            } else {
                if (scalar(grep {defined $targetstr{$_}{$str}} @targets) != scalar(@targets)) {
                    #print generic if not all targetshave a translation
                    print $fh ":$str\n$strings{$str}\n";
                }
                foreach my $key (sort @targets) {
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

