#!/usr/bin/env perl
use Getopt::Long;

my $FILEOBJ_DIR = 0x7F;
my $FILEOBJ_FILE = 0xF7;
my $START_SECTOR = 0xFF;

my $outf = "";
my $invert = 0;

sub add_dir {
    my($name, $parent, $id) = @_;
    my ($root, $ext) = split(/\./, $name);
    $outf .= pack("C C a8 a3 C C C", $FILEOBJ_DIR, $parent, $root, $ext, $id, 0, 0);
}

sub add_file {
    my($name, $parent, $data) = @_;
    my ($root, $ext) = split(/\./, $name);
    my $size = length($data);
    $outf .= pack("C C a8 a3 C C C a*", $FILEOBJ_FILE, $parent, $root, $ext, $size >> 16, 0xff & ($size >> 8), $size & 0xff, $data);
}
GetOptions("invert" => \$invert);

my $root = shift(@ARGV);
my $next_dir = 1;
my %dirid;
my (@files) = map {s/^..//; $_} split(/\n/, `chdir $root; find . -type f`);
foreach my $file (@files) {
    my @dirs = split(/\//, $file);
    my $filename = pop @dirs;
    my $cur_dir = 0;
    foreach my $dir (@dirs) {
        if(! $dirid{$dir}) {
            $dirid{$dir} = $next_dir++;
            add_dir($dir, $cur_dir, $dirid{$dir});
        }
        $cur_dir = $dirid{$dir};
    }
    my $data;
    {
        local $/=undef;
        open my $fh, "<", $root . "/" . $file;
        binmode $fh;
        $data = <$fh>;
        close $fh;
    }
    add_file($filename, $cur_dir, $data);
}

my @data = split(//, $outf);
my $free_space = 65536-4096-scalar(@data);
printf STDERR "Devo5s Size:  %dkB.  Free Space: %dkB\n", scalar(@data)/1024, $free_space / 1024;
die "Not enough free spacei (at least 2kB required)\n" if ($free_space < 2048); 
my $next_sec = $START_SECTOR;
for my $i (0 .. 65535) {
    if (! @data || $i >= 65536 - 4096) {
        printf("%c", $invert ? 0xff : 0x00);
        next;
    }
    if(! ($i % 4096)) {
        printf("%c", $invert ? 0xff - $next_sec : $next_sec);
        $next_sec = 1;
        next;
    }
    $val = shift(@data);
    $val = chr(0xff-ord($val)) if ($invert);
    print $val;
}

