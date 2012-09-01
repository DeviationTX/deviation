#!/usr/bin/env perl
use strict;
use warnings;

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
my %uniq = map {$_ => 1} @out;
foreach (sort keys %uniq) {
    print ":$_\n";
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

