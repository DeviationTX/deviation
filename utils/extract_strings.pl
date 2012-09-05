#!/usr/bin/env perl
use strict;
use warnings;

use Getopt::Long;
my $update;

GetOptions("update" => \$update);
my @files = `find . -name "*.[hc]" | xargs xgettext -o - --omit-header -k --keyword=_tr --keyword=_tr_noop --no-wrap`;
my @out;
foreach (@files) {
    next unless(/^(msgid|")/);
    if(/^msgid/) {
        push @out, "";
    }
    s/^.*?"(.*)"\s*$/$1/;
    $out[-1] .= $_;
}
foreach (`head -n 1 filesystem/template/tmpl*.ini`) {
    chomp;
    if(/template=(.*?)\s*$/) {
        push @out, $1;
    }
}
my %uniq = map {$_ => undef} @out;

if(! $update) {
    foreach (sort keys %uniq) {
        print ":$_\n";
    }
    exit 0;
}

foreach my $file (glob "filesystem/language/lang*") {
    my %strings = %uniq;
    my %unused;
    open my $fh, "<", $file;
    my $name = <$fh>;
    while(<$fh>) {
        chomp;
        if(/^:(.*)/) {
            my $next = <$fh>;
            chomp $next;
            if (! exists $uniq{$1}) {
                $unused{$1} = 1;
            }
            $strings{$1} = $next;
        }
    }
    open $fh, ">", $file;
    print $fh $name;
    foreach (sort keys %strings) {
        if ($unused{$_}) {
            print $fh "<:$_\n<$strings{$_}\n";
        } elsif(! defined($strings{$_})) {
            print $fh ">:$_\n";
        } else {
            print $fh ":$_\n$strings{$_}\n";
        }
    }
    close $fh;
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

