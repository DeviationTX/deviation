#!/usr/bin/perl
use strict;
use warnings;
use Data::Dumper;
use Getopt::Long;

my %mem;
my %children;
my %tree;
my $start;
my $total;

my $showtree = 0;
my $match = "";
my $rom;
GetOptions("matchfunc=s" => \$match, "showtree" => \$showtree, "rom" => \$rom);

my $target = shift(@ARGV);
read_mapfile();
if($showtree) {
    read_listfile();
    build_func_tree();
}

sub build_func_tree {
    foreach my $func (keys %mem) {
        %tree = ();
        $tree{$func} = {};
        get_children($tree{$func}, $func);
        my @lines = show_tree($tree{$func}, "  ");
        print("$func: " . get_size($tree{$func}, $func) . "\n@lines");
    }
}
sub show_tree {
    my($treeptr, $indent) = @_;
    my $size = 0;
    my @lines = ();
    foreach my $child(sort keys %$treeptr) {
        push @lines, "$indent$child: " . get_size($treeptr->{$child}, $child) . "\n";
        push @lines, show_tree($treeptr->{$child}, $indent . "  ");
    }
    return @lines;
}
sub get_size {
    my($treeptr, $func, $seen) = @_;
    $seen = [] if ! $seen;
    my $size = $mem{$func} || 0;
    foreach my $child(sort keys %$treeptr) {
        next if(grep {$child eq $_} @$seen);
        push @$seen, $child;
        $size += get_size($treeptr->{$child}, $child, $seen);
    }
    return $size;
}
        
sub get_children {
    my($treeptr, @seen) = @_;
    foreach my $child (@{ $children{$seen[0]} }) {
        if(grep {$child eq $_} @seen) {
            $treeptr->{"$child RECURSIVE"} = undef;
        } else {
            $treeptr->{$child} = {};
            get_children($treeptr->{$child}, $child, @seen);
        }
    }
} 
sub read_listfile {
    open my $fh, "<", "$target.list" or die "Couldn't read $target.list\n";
    my $token;
    while(<$fh>) {
        if(/^[0-9a-f]+ <(\S+)>:$/) {
            $token = $1;
            $children{$token} = [];
        } elsif(/\s[0-9a-f]+\s<(\S+)>$/) {
            my $func = $1;
            next if ($func =~ /\+|-/ || $func eq $token);
            push @{ $children{$token} }, $func unless(grep {$_ eq $func} @{ $children{$token} });
        }
    }
}

sub set_mem {
    my($function, $mem, $obj) = @_;
    if ($obj =~ /\((\S+)\.0\)/) {
        $obj = $1;
    } else {
        $obj =~ s/^.*\///;
        $obj =~ s/\.o$//;
    }
    $mem{"$function:$obj"} = $mem;
}

sub read_mapfile {
    open my $fh, "<", "$target.map" or die "Couldn't read $target.map\n";
    while(<$fh>) {
        if(! $start) {
            if(/^\.text\s+0x\S+\s+(0x\S+)/) {
                $start = 1;
                $total = hex($1);
            }
            next;
        }
        if(/^\s*\.text\.(\S+)/) {
            my $function = $1;
            if(/0x\S+\s+0x(\S+)\s+(\S+)/) {
                set_mem($function, hex($1), $2);
            } else {
                $_ = <$fh>;
                if(/0x\S+\s+0x(\S+)\s(\S+)/) {
                    set_mem($function, hex($1), $2);
                } else {
                    printf("Couldn't identify memory usage for $function at line $.\n");
                }
            }
        } elsif(/^\s*\.text\s+0x\S+\s+0x(\S+)\s+(\S+)/) {
            my $size = $1;
            my $obj = $2;
            $_ = <$fh>;
            if(/^\s+0x\S+\s+(\S+)/) {
                set_mem($1, hex($size), $obj);
            } else {
                printf("Couldn't identify function from: $_ at line $.\n");
            }
        }
        next if(! $rom);
        if(/^\s*\.rodata\.(\S+)/) {
            my $var = $1;
            if(/0x\S+\s+0x(\S+)\s+(\S+)/) {
                set_mem("ROM_" . $var, hex($1), $2);
            } else {
                $_ = <$fh>;
                if(/0x\S+\s+0x(\S+)\s(\S+)/) {
                    set_mem("ROM_" . $var, hex($1), $2);
                } else {
                    printf("Couldn't identify memory usage for $var at line $.\n");
                }
            }
        } elsif(/^\s*\.rodata\s+0x\S+\s+0x(\S+)\s+(\S+)/) {
            my $size = $1;
            my $obj = $2;
            set_mem("ROM_UNKNOWN", hex($size), $obj);
        }
    }
        
    my $funcsize = 0;
    my $romsize = 0;
    foreach my $func (sort keys %mem) {
        next if($match && $_ !~ $func =~ /$match/);
        my ($base) = ($func =~ /^(\S+):/);
        if (scalar(grep { /^$base/ } keys(%mem)) > 1) {
            $base = $func;
        }
        printf("%10d $base\n", $mem{$func});
        if($func =~ /ROM_/) {
            $romsize += $mem{$func};
        } else {
            $funcsize+= $mem{$func};
        }
    }
    if(! $rom) {
        $romsize = $total - $funcsize;
    }
    printf("%10d TOTAL function size\n", $funcsize);
    printf("%10d ROM data\n", $romsize);
    printf("%10d Unknown data\n", $total - $funcsize - $romsize) if($rom);
    printf("%10d TOTAL\n", $total);
}

