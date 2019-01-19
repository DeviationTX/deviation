#!/usr/bin/env perl
use strict;
use warnings;

use Getopt::Long;

my @requested_targets;
# The following are legal alternatives to the default string
my @targets = ("devo8", "devo10", "devo12");
my $PO_LANGUAGE_STRING = "<Translated Language Name>";

sub main {
    my $update;
    my $lang;
    my $fs;
    my $target_list;
    my $objdir;
    my $elffile;
    my $count;
    my $po;

    $ENV{CROSS} ||= "";
    GetOptions("update" => \$update, "language=s" => \$lang, "fs=s" => \$fs,
        "targets=s" => \$target_list, "count" => \$count, "po" => \$po,
        "objdir=s" => \$objdir, "elffile=s" => \$elffile);
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

    my $uniq = get_strings($elffile || $objdir);
    if($count) {
        printf "%d", scalar(keys(%$uniq));
        exit 0;
    }

    if(! $update) {
        foreach (sort keys %$uniq) {
            print ":$_\n";
        }
        exit 0;
    }

    my @files = ($po ?
                 ($lang ?
                     "fs/language/$lang.po" :
                     glob("fs/language/*.po")) :
                 glob($lang ?
                     "fs/language/lang*.$lang" :
                     "fs/language/lang*"));
    for my $file (@files) {
        my($language, $translation) = $po
            ? parse_po_file($file, $uniq)
            : parse_v1lang_file($file, $uniq);
        my ($ext) = ($file =~ /[^\/.]+\.([^\/.\s]+)(?:\.po)?$/);
        write_lang_file("$fs/lang.$ext", $language, $translation);
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
            $strings->{$1} = 1;
        }
    }
    return $strings;
}

sub extract_all_strings {
    my @lines = `/usr/bin/find . -name "*.[hc]" | grep -v libopencm3 | xargs xgettext -o - --omit-header -k --keyword=_tr --keyword=_tr_noop --no-wrap`;
    my $idx = 0;
    my %strings;
    #Read all strings and put into a hash that maps the containing file to the string
    while(@lines) {
        my($msgid, $msgstr) = parse_gettext(\@lines);
        next unless($msgid);
        $strings{$msgid} = 1;
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
                            $strings{$str} = 1 if($valid_strings->{$str});
                            $str = "";
                        }
                    } else {
                        $str .= chr(hex($1));
                    }
                }
            }
        }
    }
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
    my($outf, $language, $translation) = @_;
    my %strings;
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
    }

    open(my $fh, ">", $outf) or die "ERROR: Can't write $outf\n";
    print $fh $language;
    foreach(sort{$a <=> $b} keys %hashvalues)
    {
        print $fh pack("CC", $_ & 0xFF, $_ >> 8);
        print $fh "$hashvalues{$_}\n";
    }
    close $fh;
}

sub parse_gettext {
    my($lines) = @_;
    my $msgid;
    while(1) {
        my $line = shift(@$lines);
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
                if (! $msgid) {
                    print "Error: no msgid for msgstr '$str'.  Ignoring\n";
                    next;
                }
                return($msgid, $str);
            }
        }
    }
}

sub parse_po_file {
    my($file, $uniq) = @_;
    my %strings;
    my $language = "Unknown";
    my $re_target = join("|", @targets);

    open my $fh, "<", $file;
    my @lines = <$fh>;
    close($fh);
    while(@lines) {
        my($msgid, $msgstr) = parse_gettext(\@lines);
        next if(! $msgid || ! $uniq->{$msgid});
        if ($msgid eq $PO_LANGUAGE_STRING) {
            $language = $msgstr;
            next;
        }
        my @values = split(/\n(?=(?:$re_target):)/, $msgstr);
        foreach (@values) {
            my($target, $target_str) = (/^(?:($re_target):)(.*)/);
            $strings{$target}{$msgid} = $target_str if($target);
            $strings{DEFAULT}{$msgid} |= $target_str;
        }
    }
    return ($language, \%strings);
}

sub parse_v1lang_file {
    my($file, $uniq) = @_;
    my %strings;
    open my $fh, "<", $file;
    my $language = <$fh>;
    my(@lines) = <$fh>;
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
    return($language, \%strings);
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

