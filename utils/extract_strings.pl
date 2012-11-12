#!/usr/bin/env perl
use strict;
use warnings;

use Getopt::Long;
my $update;
my $lang;
my $target;
my @targets = ("320x240x16", "128x64x1");
my %targetmap = (
    devo8 => "320x240x16",
    devo10 => "128x64x1",
);

GetOptions("update" => \$update, "language=s" => \$lang, "target=s" => \$target);
if($target && (! $targetmap{$target} || ! (grep {$targetmap{$target} eq $_} @targets))) {
    my @t = keys(%targetmap);
    print "Target must be one of: @t\n";
    exit 1;
}
my @lines = `find . -name "*.[hc]" | xargs xgettext -o - --omit-header -k --keyword=_tr --keyword=_tr_noop --no-wrap`;
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
    if(/#:\s+(\S+):\d+$/) {
        push @files, "$1:$idx";
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
if($target) {
    foreach my $f (keys %filemap) {
        if(grep {$_ ne $targetmap{$target} && $f =~ /$_/} @targets) {
            delete $filemap{$f};
        }
    }
}
#build string list
my@out = values(%filemap);
#append template names
foreach (`head -n 1 filesystem/common/template/*.ini`) {
    chomp;
    if(/template=(.*?)\s*$/) {
        push @out, $1;
    }
}
my %uniq = map {$_ => undef} @out;

if(! $update && ! $target) {
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
    my %unused;
    open my $fh, "<", $file;
    my $name = <$fh>;
    while(<$fh>) {
        chomp;
        if(/^:(.*)/) {               #/^([<>]?):(.*)/)
            my($eng) = ($1);
            my $next = <$fh>;
            chomp $next;
            #($next) = ($next =~ /^.(.*)/) if ($missing);
            if (! exists $uniq{$eng}) {
                $unused{$eng} = 1;
            }
            $strings{$eng} = $next;
        }
    }
    if($target) {
        #if target is specified, we want to return a filtered list of strings
        my $outf = $file;
        $outf =~ s/common/$target/;
        open $fh, ">", $outf;
        print $fh $name;
        foreach (sort keys %strings) {
            if(! $unused{$_} && defined($strings{$_})) {
                print $fh ":$_\n$strings{$_}\n";
            }
        }
        close $fh;
    } else {
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

