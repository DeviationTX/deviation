#!/usr/bin/env perl

# Before 'make install' is performed this script should be runnable with
# 'make test'. After 'make install' it should work as 'perl DevoFS.t'

#########################

use strict;
use warnings;

use ExtUtils::testlib;
use FindBin;
use File::Temp;
use Digest::MD5;
use Fcntl;
use Data::Dumper;

use Test::More tests => 181;
BEGIN { use_ok('DevoFS') };

#########################

my $fat;
my $image;
my @imgfiles;
my %files;
my $tmpdir = File::Temp::tempdir(CLEANUP => 1);
my $imgdir = $FindBin::Bin . "/../image";
my $MAX_FILE_SIZE = 65535;

#tests here
init();
verify_files();
read_partial();
lseek();
write_file();
auto_compact();
manual_compact();
change_filesize();
write_around_the_horn();

sub msg
{
        my($msg) = @_;
        my @a = caller(1);
        $a[3] =~ s/^.*:://;
        $a[3] .= " - $msg" if($msg);
        return $a[3];
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

sub _compare_fs {
    my($ref, $msg) = @_;
    my $parent = (caller(1))[3];
    $parent =~ s/^.*:://;
    $parent .= " - $msg" if($msg);
    my %new = _read_all_files("");
    my @ref_files = sort(keys(%$ref));
    my @new_files = sort(keys(%new));
    ok(eq_array(\@ref_files, \@new_files), "$parent - Matched file list");
    foreach my $file (sort keys %$ref) {
        is(DevoFS::open($file, 0), 0, "$parent - Opened $file");
        my $data1 = "";
        my $len = 0;
        my $ret = DevoFS::read($data1, $MAX_FILE_SIZE, $len);
        is($len, $ref->{$file}{SIZE}, "$parent - Read expected number of bytes from $file");
        my $new_md5 = Digest::MD5::md5_hex($data1);
        $new{$file}{MD5} = $new_md5;
        $new{$file}{DATA} = $data1;
        is($new_md5, $ref->{$file}{MD5}, "$parent - $file matches reference");
    }
    return %new;
}

#######################################################################################################
sub init {
    @imgfiles = split(/\n/, `chdir $imgdir; find . -type f`);
    foreach (@imgfiles) {
        s/^\.\///;
    }
    @imgfiles = sort(@imgfiles);
    $image = $tmpdir . "/devofs.img";
    ok(system("python $FindBin::Bin/../../buildfs.py --dir $imgdir --fs $image -c") == 0, "Built filesystem");
    system("cp $image $image.1");
    $fat = DevoFS::mount("$image.1");
    ok($fat, "Mounted image");
    return;
}

sub verify_files {
    my %ref;
    foreach my $file (@imgfiles) {
        my $data;
        {
            local $/=undef;
            open my $fh, "<", "$imgdir/$file";
            binmode $fh;
            $data = <$fh>;
            close $fh;
        }
        my $ref_md5 = Digest::MD5::md5_hex($data);
        $ref{$file} = {SIZE => length($data), MD5 => $ref_md5};
    }
    %files = _compare_fs(\%ref);
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
    DevoFS::open("protocol/devo.mod", O_CREAT);
    is(DevoFS::write($data, 4096, $len), 0, msg("Write successful"));
    is($len, 4096, msg("Wrote correct length"));
    DevoFS::close();
    DevoFS::open("protocol/devo.mod", 0);
    is(DevoFS::read($data1, 4096, $len), 0, msg("Read written data"));
    is($len, 4096, msg("Read proper length"));
    my $new_md5 = Digest::MD5::md5_hex($data1);
    $files{"protocol/devo.mod"}{DATA} = $data1;
    $files{"protocol/devo.mod"}{MD5} = $new_md5;
    is($new_md5, $ref_md5, msg("Written data is correct"));
}

sub change_filesize {
    for my $size (20, 2000, 0, 1, 4096-16, 15) {
        my $len = -1;
        my $data = "";
        my $rdata = "";
        for (1 .. $size) { $data .= chr( 32 + int(rand(95)) ); } #Only printable chars
        $files{"models/model10.ini"}{MD5} = Digest::MD5::md5_hex($data);
        DevoFS::open("models/model10.ini", O_CREAT);
        is(DevoFS::write($data, $size, $len), 0, msg("Write successful: $size"));
        DevoFS::close();
        is($len, $size, msg("Wrote proper length: $size"));
        DevoFS::open("models/model10.ini", 0);
        $len = -1;
        is(DevoFS::read($rdata, 4096, $len), 0, msg("Read written data"));
        is($len, $size, msg("File size is as expected"));
        is(Digest::MD5::md5_hex($rdata), $files{"models/model10.ini"}{MD5}, msg("Data matched"));
    }
    # We round max-size tothe nearestsector includeing the header
    my $len = -1;
    my $data = "";
    my $rdata = "";
    my $expected_size = 4096-16;
    for (1 .. 4096) { $data .= chr( 32 + int(rand(95)) ); } #Only printable chars
    $files{"models/model10.ini"}{MD5} = Digest::MD5::md5_hex(substr($data, 0, $expected_size));
    DevoFS::open("models/model10.ini", O_CREAT);
    is(DevoFS::write($data, 4096, $len), 0, msg("Write successful: 4096"));
    DevoFS::close();
    is($len, $expected_size, msg("Wrote proper length: 4096 (actual: $expected_size)"));
    DevoFS::open("models/model10.ini", 0);
    $len = -1;
    is(DevoFS::read($rdata, 4096, $len), 0, msg("Read written data"));
    is($len, $expected_size, msg("File size is as expected"));
    is(Digest::MD5::md5_hex($rdata), $files{"models/model10.ini"}{MD5}, msg("Data matched"));
}

sub auto_compact {
    my $data = "";
    my $len = 0;
    for my $i (0 .. 12) {
        #20 iterations is enough to cause auto-compaction
        DevoFS::open("protocol/devo.mod", O_CREAT);
        DevoFS::write($files{"protocol/devo.mod"}{DATA}, 4096, $len);
        DevoFS::close();
    }
    _compare_fs(\%files);
}

sub manual_compact {
    for my $i (0 .. 3) {
        DevoFS::compact();
        _compare_fs(\%files, $i);
    }
}

sub write_around_the_horn {
    #Start with a clean database
    system("cp $image $image.2");
    $fat = DevoFS::mount("$image.2");
    #
    #compact thrice (this will move the start 'before the horn)
    DevoFS::compact();
    DevoFS::compact();
    DevoFS::compact();
    #Write mod file (this will go around the horn
    my $len = 0;
    DevoFS::open("protocol/devo.mod", O_CREAT);
    is(DevoFS::write($files{"protocol/devo.mod"}{DATA}, 4096, $len), 0, msg("Wrote mod file"));
    DevoFS::close();
    #
    my $data = "";
    DevoFS::open("protocol/devo.mod", 0);
    is(DevoFS::read($data, 4096, $len), 0, msg("Read written data"));
    is(Digest::MD5::md5_hex($data), $files{"protocol/devo.mod"}{MD5}, msg("Data matched"));
}
