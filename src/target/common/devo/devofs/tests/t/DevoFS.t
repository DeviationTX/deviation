#!/usr/bin/perl

# Before 'make install' is performed this script should be runnable with
# 'make test'. After 'make install' it should work as 'perl DevoFS.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use warnings;

use ExtUtils::testlib;
use FindBin;
use File::Temp;
use Digest::MD5;
use Data::Dumper;

use Test::More tests => 1;
BEGIN { use_ok('DevoFS') };

#########################

my $O_WRONLY = 4;

my $fat;
my $image;
my @imgfiles;
my %files;
my $tmpdir = File::Temp::tempdir(CLEANUP => 1);
my $imgdir = $FindBin::Bin . "/../image";
my $MAX_FILE_SIZE = 65535;

#tests here
init();
%files = verify_files();
read_partial();
lseek();
#write_file();
auto_compact();

sub msg
{
        my($msg) = @_;
        my @a = caller(1);
        $a[3] =~ s/^.*:://;
        $a[3] .= " - $msg" if($msg);
        return $a[3];
}

sub init {
    @imgfiles = split(/\n/, `chdir $imgdir; find . -type f`);
    foreach (@imgfiles) {
        s/^\.\///;
    }
    @imgfiles = sort(@imgfiles);
    $image = $tmpdir . "/devofs.img";
    ok(system("perl $FindBin::Bin/../buildfs.pl $imgdir > $image") == 0, "Built filesystem");
    $fat = DevoFS::mount($image);
    ok($fat, "Mounted image");
    return;
}

sub _read_all_files {
    my($path) = @_;
    my $dir = DevoFS::opendir($path);
    die "Failed to open '$path'\n" if(! $dir);
    my %files;
    while(1) {
        my @fi = DevoFS::readdir($dir);
        last if (scalar(@fi) == 1);
        if ($fi[1] == 0) {
            %files = (%files, _read_all_files($path ? $path . "/" . $fi[0] : $fi[0]));
        } else {
            $files{$path ? "$path/$fi[0]" : $fi[0]} = { SIZE => $fi[1]};
        }
    }
    return %files;
}

sub verify_files {
    my %files = _read_all_files("");
    my @dirs = sort(keys %files);
    ok(eq_array(\@dirs, \@imgfiles), msg("Matched file list"));
    foreach my $file (@dirs) {
        my $len = 0;
        my $data;
        {
            local $/=undef;
            open my $fh, "<", "$imgdir/$file";
            binmode $fh;
            $data = <$fh>;
            close $fh;
        }
        my $ref_md5 = Digest::MD5::md5_hex($data);
        is(DevoFS::open($file, 0), 0, msg("Opened $file"));
        my $data1 = "";
        my $ret = DevoFS::read($data1, $MAX_FILE_SIZE, $len);
        is($ret, 0, msg("Read $file"));
        is($len, $files{$file}{SIZE}, msg("Read expected number of bytes from $file"));
        ok(\$data1, msg("Read $file"));
        $files{$file}{DATA} = $data1;
        my $new_md5 = Digest::MD5::md5_hex($data1);
        $files{$file}{MD5} = $new_md5;
        is($ref_md5, $new_md5, msg("Data matches reference"));
    }
    return %files;
}

sub read_partial {
    my $data = "";
    my $len = 0;
    DevoFS::open("hardware.ini", 0);
    DevoFS::read($data, 13, $len);
    DevoFS::read($data, 3, $len);
    is($data, "for", msg("partial-read"));
}

sub lseek {
    my $data = "";
    my $len = 0;
    DevoFS::open("hardware.ini", 0);
    is(DevoFS::lseek(0xdc), 0, msg("Lseek successful"));
    DevoFS::read($data, 9, $len);
    is($data, "alternate", msg("read after seek"));
}

sub write_file {
    my $data = "";
    my $data1 = "";
    my $len = 0;
    for (0..4095) { $data .= chr( int(rand(255)) ); }
    my $ref_md5 = Digest::MD5::md5_hex($data);
    DevoFS::open("protocol/devo.mod", $O_WRONLY);
    is(DevoFS::write($data, 4096, $len), 0, msg("Write successful"));
    is($len, 4096, msg("Wrote correct length"));
    DevoFS::open("protocol/devo.mod", 0);
    is(DevoFS::read($data1, 4096, $len), 0, msg("Read written data"));
    is($len, 4096, msg("Read proper length"));
    my $new_md5 = Digest::MD5::md5_hex($data1);
    $files{"protocol/devo.mod"}{DATA} = $data1;
    $files{"protocol/devo.mod"}{MD5} = $new_md5;
    is($new_md5, $ref_md5, msg("Written data is correct"));
}

sub auto_compact {
    my $data = "";
    my $len = 0;
    for my $i (0 .. 11) {
        #20 iterations is enough to cause auto-compaction
        DevoFS::open("protocol/devo.mod", $O_WRONLY);
        DevoFS::write($files{"protocol/devo.mod"}{DATA}, 4096, $len);
    }
    system("cp $image .");
    DevoFS::open("protocol/devo.mod", $O_WRONLY);
    DevoFS::write($files{"protocol/devo.mod"}{DATA}, 4096, $len);
    system("cp $image ./devofs.bad");
    my %newfiles = _read_all_files("");
    foreach my $file (sort keys %files) {
        ok($newfiles{$file}, msg("Found $file after compact"));
        is(DevoFS::open($file, 0), 0, msg("Opened $file"));
        my $data1 = "";
        my $ret = DevoFS::read($data1, $MAX_FILE_SIZE, $len);
        my $new_md5 = Digest::MD5::md5_hex($data1);
        is($new_md5, $files{$file}{MD5}, msg("Mismatch after compaction in $file"));
    }
}
