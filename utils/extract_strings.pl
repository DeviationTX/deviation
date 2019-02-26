#!/usr/bin/env perl
use strict;
use warnings;

use Getopt::Long;

my @requested_targets;
# The following are legal alternatives to the default string
my @targets = ("devo8", "devo10", "devo12");
my $PO_LANGUAGE_STRING = "->Translated Language Name<-";

sub main {
    my $update;
    my $lang;
    my $fs;
    my $target_list;
    my $objdir;
    my $elffile;
    my $count;
    my $po;
    my $format = "v1";

    $ENV{CROSS} ||= "";
    GetOptions("update" => \$update, "language=s" => \$lang, "fs=s" => \$fs,
        "targets=s" => \$target_list, "count" => \$count, "po" => \$po,
        "objdir=s" => \$objdir, "elffile=s" => \$elffile,
        "format=s" => \$format);
    @requested_targets = split(/,/, $target_list) if($target_list);
    check_targets($fs, @requested_targets);

    my $uniq = get_strings($elffile || $objdir);
    if($count) {
        printf "%d", scalar(keys(%$uniq));
        exit 0;
    }

    if(! $update) {
        if ($po) {
            print "msgid \"$PO_LANGUAGE_STRING\"\nmsgstr \"\"\n\n";
        }
        foreach (@{ $uniq->{__ORDER__} }) {
            if ($po) {
                # Match same syntax used by getopt
                print $uniq->{$_}; # Comment
                my @msg = split(/(?<=\\n)/, $_);
                if (scalar(@msg) > 1) {
                    print "msgid \"\"\n\"" . join("\"\n\"", @msg) . "\"\n";
                } else {
                    print "msgid \"$msg[0]\"\n";
                }
                print "msgstr \"\"\n";
                print "\n";
            } else {
                print ":$_\n";
           }
        }
        exit 0;
    }

    my @files = ($po ?
                 ($lang ?
                     "fs/language/locale/deviation.$lang.po" :
                     glob("fs/language/locale/*.po")) :
                 glob($lang ?
                     "fs/language/lang*.$lang" :
                     "fs/language/lang*"));
    for my $file (@files) {
        my($ext, $language, $translation) = $po
            ? parse_po_file($file, $uniq)
            : parse_v1lang_file($file, $uniq);
        write_lang_file("$fs/lang.$ext", $language, $translation, $format);
    }
}

sub check_targets {
    my($fs, @requested_targets) = @_;
    if($fs || @requested_targets) {
        if (! @requested_targets) {
            print "ERROR: Must specify both -targets with -fs\n";
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
}

sub get_strings {
    my($path) = @_;
    my $strings = extract_all_strings();
    if ($path) {
        $strings = extract_target_strings($path, $strings);
    }
    #append template names
    foreach (`head -n 1 fs/common/template/*.ini`) {
        chomp;
        if(/template=(.*?)\s*$/) {
            push @{ $strings->{__ORDER__} }, $1 if (! $strings->{$1});
            $strings->{$1} = "#: Model template\n";
        }
    }
    return $strings;
}

sub extract_all_strings {
    my @lines = `/usr/bin/find . -name "*.[hc]" | grep -v libopencm3 | sort | xargs xgettext -o - --omit-header -k --keyword=_tr --keyword=_tr_noop --no-wrap`;
    my $idx = 0;
    my %strings = (__ORDER__ => [] );  # Crude Tie::IxHash implementation
    #Read all strings and put into a hash that maps the containing file to the string
    while(@lines) {
        my($msgid, $msgstr, $comment) = parse_gettext(\@lines);
        next unless($msgid);
        push @{ $strings{__ORDER__} }, $msgid if(! defined $strings{$msgid});
        $strings{$msgid} = $comment;
    }
    return \%strings;
}

sub extract_target_strings {
    my($path, $valid_strings) = @_;
    my %strings;
    my @files = (-d $path ? glob("$path/*.o") : $path);

    foreach my $file (@files) {
        #Parse all strings from the object files and add to the allstr hash
        my @od = `$ENV{CROSS}objdump -s $file`;
        my $str = "";
        my $state = 0;
        foreach(@od) {
            my $orig = $_;
            if(/section (\.rel)?\.ro?data/ || /section __cstring/) {
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
                            $strings{$str} = $valid_strings->{$str} if($valid_strings->{$str});
                            $str = "";
                        }
                    } else {
                        $str .= chr(hex($1));
                    }
                }
            }
        }
    }
    $strings{__ORDER__} = [ grep { defined $strings{$_} } @{ $valid_strings->{__ORDER__} } ];
    return \%strings;
}

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


sub write_lang_file {
    my($outf, $language, $translation, $format) = @_;
    my %strings;
    my %final;
    my %hashvalues;
    foreach ("DEFAULT", @requested_targets) {
        #Hierarchically try to find best string
        next unless($translation->{$_});
        %strings = (%strings, %{$translation->{$_}});
    }
    foreach (sort keys %strings) {
        next if ($_ eq $strings{$_});
        my $hash = fnv_16($_);
        my $value = $strings{$_};
        if (defined($hashvalues{$hash})) {
            printf("Error: Conflict hash detected:\n%s\n%s\n",
                   $hashvalues{$hash}, $value);
            exit 1;
        }
        $hashvalues{$hash} = $value;
        $final{$_} = $value;
    }

    open(my $fh, ">", $outf) or die "ERROR: Can't write $outf\n";
    print $fh $language;
    if ($format eq "v2") {
        foreach(sort{$a <=> $b} keys %hashvalues)
        {
            print $fh pack("CC", $_ & 0xFF, $_ >> 8);
            print $fh "$hashvalues{$_}\n";
        }
    } else {
       foreach (sort keys %final) {
           print $fh ":$_\n$final{$_}\n";
       }
    }
    close $fh;
}

sub parse_gettext {
    my($lines) = @_;
    my $msgid;
    my $comment = "";
    while(1) {
        my $line = shift(@$lines);
        if ($line =~ /^#[:,]/) {
            $comment .= $line;
            next;
        }
        if ($line =~ /^\s*(msgid|msgstr)\s+"(.*)"\s*$/) {
            my($type, $str) = ($1, $2);
            while(@$lines && $lines->[0] =~ /^\s*"(.*)"\s*$/) {
                $str .= $1;
                shift(@$lines);
            }
            if ($type eq "msgid") {
                if ($msgid) {
                    print "Error: msgid '$msgid' missing msgstr.  Ignoring\n";
                }
                $msgid = $str;
            }
            elsif ($type eq "msgstr") {
                if (! defined $msgid) {
                    print "Error: no msgid for msgstr '$str'.  Ignoring\n";
                    next;
                }
                return($msgid, $str, $comment || "#");
            }
        }
    }
}

sub parse_po_file {
    my($file, $uniq) = @_;
    my %strings;
    my $language = "Unknown";
    my $ext;
    my $re_target = join("|", @targets);

    open my $fh, "<", $file;
    my @lines = <$fh>;
    close($fh);
    while(@lines) {
        my($msgid, $msgstr) = parse_gettext(\@lines);
        $msgstr =~ s/\\([^n])/$1/g; # Fix escaped characters
        next if(! $msgstr);
        if ($msgid eq "" && $msgstr =~ /(?:\b|\\n)Language:\s+(\S+?)(?:\b|\\n)/) {
            $ext = lc($1);
            if (length($ext) > 3) {
                $ext =~ s/^.*_//;
                $ext = substr($ext, -3);
            }
        }
        next if(! $msgid);
        if ($msgid eq $PO_LANGUAGE_STRING) {
            # Add UTF-8 BOM
            $language = chr(0xef) . chr(0xbb) . chr(0xbf) . $msgstr . "\n";
            next;
        }
        next if(! $uniq->{$msgid});
        my @values = split(/\\n(?=(?:$re_target)?:)/, $msgstr);
        foreach (@values) {
            my($target, $target_str) = (/^(?:($re_target):)?(.*)/);
            $strings{$target}{$msgid} = $target_str if($target);
            $strings{DEFAULT}{$msgid} ||= $target_str;
        }
    }
    return ($ext, $language, \%strings);
}

sub parse_v1lang_file {
    my($file, $uniq) = @_;
    my %strings;
    open my $fh, "<", $file;
    my $language = <$fh>;
    my(@lines) = <$fh>;
    my ($ext) = ($file =~ /[^\/.]+\.([^\/.\s]+)(?:\.po)?$/);
    for (my $line = 0; $line < $#lines; $line++) {
        $_ = $lines[$line];
        chomp;
        if(/^:(.*)/) {               #/^([<>]?):(.*)/)
            my($eng) = ($1);
            next if ($line < $#lines-2 && $lines[$line+1] =~ /^:./);
            my $next = $lines[++$line];
            chomp $next;
            $strings{DEFAULT}{$eng} = $next if($uniq->{$eng});
        } elsif(/^\|(\S+?):(.*)/) {
            my($t, $eng) = ($1, $2);
            my $next = $lines[++$line];
            chomp $next;
            next if(! $uniq->{$eng});
            $strings{$t}{$eng} = $next;
            $strings{DEFAULT}{$eng} ||= $next;
        }
    }
    return($ext, $language, \%strings);
}

main();

# | xargs xgettext -o - --omit-header -k --keyword=_tr --keyword=_tr_noop --no-wrap | grep -v 'msgstr ""' | grep -v "^#" | grep -v "^$"

#: pages/dialogs.c:100
#msgid ""
#"Binding is in progress...\n"
#"Make sure model is on!\n"
#"\n"
#"Pressing OK will NOT cancel binding procedure\n"
#"but will allow full control of Tx."
#msgstr ""

