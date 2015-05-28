#!/usr/bin/env perl
use warnings;
use strict;

my @files = grep {$_ !~ /protocol.c/} glob("protocol/*.c");
my @target = ();
print "ifndef BUILD_TARGET\n\n";
foreach (@files) {
    s/^.*\/(\S+)\.c$/$1/;
    my $cmd_name = `grep PROTO_Cmds protocol/$_.c | sed -e 's/^.* \\(.*\\) PROTO_Cmds/\\1/' | tr -d '\\n'`;
    my @out_names = split(/\n/, `grep $cmd_name protocol/protocol.h | sed -e 's/^.*"\\(.*\\)".*/\\1/' | tr A-Z a-z`);
    foreach my $n (@out_names)  {
        print "PROTO_MODULES += \$(ODIR)/protocol/$n.mod\n";
        push @target, "\$(ODIR)/protocol/$n.mod : \$(ODIR)/$_.bin\n";
        push @target, "\t\@echo Building '$n' module\n";
        push @target, "\t/bin/mkdir -p \$(ODIR)/protocol/ 2>/dev/null; /bin/cp \$< \$@\n";
        push @target, "\n";
    }
}
print "ALL += \$(PROTO_MODULES)\n";
print "else #BUILD_TARGET\n";
print @target;
print "endif #BUILD_TARGET\n";
