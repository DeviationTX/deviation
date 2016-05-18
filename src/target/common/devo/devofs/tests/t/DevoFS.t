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

use Test::More tests => 213;
BEGIN { use_ok('DevoFS') };

#########################

my $fat;
my $image;
my @imgfiles;
my %files;
my $img_idx = 0;
my $tmpdir = File::Temp::tempdir(CLEANUP => 1);
my $imgdir = $FindBin::Bin . "/../image";
my $MAX_FILE_SIZE = 65535;

#tests here
init();
read_partial();
lseek();
write_file(4096);
write_file(14325);
write_file(212);
auto_compact();
manual_compact();
change_filesize();
file_sector_align();
write_around_the_horn();

sub msg
{
        my($msg) = @_;
        my @a = caller(1);
        $a[3] =~ s/^.*:://;
        $a[3] .= " - $msg" if($msg);
        return $a[3];
}

sub _update_filestats
{
    my($files, $name, $data) = @_;
    $files->{$name} = {DATA => $data, SIZE => length($data), MD5 => Digest::MD5::md5_hex($data) };
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
    my @mismatch = ();
    my @ref = ();
    foreach my $file (sort keys %$ref) {
        my $data1 = "";
        my $len = 0;
        my $open = DevoFS::open($file, 0);
        if ($open != 0) {
            push @mismatch, "\n$file could not be found on FS";
            next;
        }
        my $ret = DevoFS::read($data1, $MAX_FILE_SIZE, $len);
        my $new_md5 = Digest::MD5::md5_hex($data1);
        $new{$file}{MD5} = $new_md5;
        $new{$file}{DATA} = $data1;
        if ($len != $ref->{$file}{SIZE}) {
            push @mismatch, "\n$file expected size: $ref->{$file}{SIZE} <> actual size: $len";
        } elsif ($new_md5 ne $ref->{$file}{MD5}) {
            push @mismatch, "\n$file expected MD5: $ref->{$file}{MD5} <> actual MD5: $new_md5";
        }
    }
    my $mismatch = join("", @mismatch);
    is($mismatch, "", "$parent - Filesystem matches reference");
    return %new;
}

# Generate refernce checksums for all files
sub _build_fs_cache {
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
        _update_filestats(\%ref, $file, $data);
    }
    %files = %ref;
}

sub _reset_fs {
    $img_idx++;
    system("cp $image $image.$img_idx");
    $fat = DevoFS::mount("$image.$img_idx");
    _build_fs_cache();
}

#######################################################################################################
#Generate devo.fs image
sub init {
    @imgfiles = split(/\n/, `chdir $imgdir; find . -type f`);
    foreach (@imgfiles) {
        s/^\.\///;
    }
    @imgfiles = sort(@imgfiles);
    $image = $tmpdir . "/devofs.img";
    ok(system("python $FindBin::Bin/../../buildfs.py --dir $imgdir --fs $image -c") == 0, "Built filesystem");
    _reset_fs();
    ok($fat, "Mounted image");
    _compare_fs(\%files);
    return;
}


# Read a few bytes from a given file an verify
sub read_partial {
    my $data = "";
    my $len = 0;
    DevoFS::open("hardware.ini", 0);
    DevoFS::read($data, 13, $len);
    DevoFS::read($data, 3, $len);
    is($data, "for", msg("partial-read"));
}

# Seek to a position in a file and verify data read
sub lseek {
    my $data = "";
    my $len = 0;
    DevoFS::open("hardware.ini", 0);
    is(DevoFS::lseek(0xdc), 0, msg("Lseek successful"));
    DevoFS::read($data, 9, $len);
    is($data, "alternate", msg("read after seek"));
}

sub write_file {
    my($size) = @_;
    my $data = "";
    my $data1 = "";
    my $len = 0;
    for (0..($size-1)) { $data .= chr( int(rand(255)) ); }
    _update_filestats(\%files, "protocol/devo.mod", $data);
    DevoFS::open("protocol/devo.mod", O_CREAT);
    is(DevoFS::write($data, $size, $len), 0, msg("Write successful ($size)"));
    is($len, $size, msg("Wrote correct length ($size)"));
    DevoFS::close();
    DevoFS::open("protocol/devo.mod", 0);
    is(DevoFS::read($data1, $size, $len), 0, msg("Read written data ($size)"));
    is($len, $size, msg("Read proper length ($size)"));
    my $new_md5 = Digest::MD5::md5_hex($data1);
    is($new_md5, $files{"protocol/devo.mod"}{MD5}, msg("Written data is correct ($size)"));
}

sub change_filesize {
    for my $size (20, 2000, 0, 1, 4096-16, 8192, 15) {
        my $len = -1;
        my $data = "";
        my $rdata = "";
        for (1 .. $size) { $data .= chr( 32 + int(rand(95)) ); } #Only printable chars
        _update_filestats(\%files, "models/model10.ini", $data);
        DevoFS::open("models/model10.ini", O_CREAT);
        is(DevoFS::write($data, $size, $len), 0, msg("Write successful: $size"));
        DevoFS::close();
        is($len, $size, msg("Wrote proper length: $size"));
        DevoFS::open("models/model10.ini", 0);
        $len = -1;
        is(DevoFS::read($rdata, int(($size + 4095) / 4096) * 4096, $len), 0, msg("Read written data"));
        is($len, $size, msg("File size is as expected"));
        is(Digest::MD5::md5_hex($rdata), $files{"models/model10.ini"}{MD5}, msg("Data matched"));
    }
}

#  Align file to sector boundary
sub file_sector_align {
    my $header_size = DevoFS::sizeof_fileheader();
    foreach my $align (-$header_size-1 .. $header_size+1) {
        _reset_fs();
        my $data = "";
        my $len = -1;
        my $ptr = DevoFS::_get_next_write_addr();
        # Set pointer to next file location
        my $delta = 4096 - ($ptr % 4096);
        if ($delta == 0) {
            $delta = 4096;
        }
        $delta -= DevoFS::sizeof_fileheader(); # header size
        $delta += $align;
        for (1 .. $delta) { $data .= chr( 32 + int(rand(95)) ); } #Only printable chars
        _update_filestats(\%files, "models/model10.ini", $data);
        DevoFS::open("models/model10.ini", O_CREAT);
        is(DevoFS::write($data, $delta, $len), 0, msg("($align) Write alignment successful : $delta"));
        DevoFS::close();
        $delta = 256;
        $data = "";
        for (1 .. $delta) { $data .= chr( 32 + int(rand(95)) ); } #Only printable chars
        _update_filestats(\%files, "models/default.ini", $data);
        DevoFS::open("models/default.ini", O_CREAT);
        $len = -1;
        is(DevoFS::write($data, $delta, $len), 0, msg("($align) Write adjacent successful: $delta"));
        DevoFS::close();
        _compare_fs(\%files, "($align - $img_idx)");
    }
}

sub auto_compact {
    my $data = "";
    my $len = 0;
    _reset_fs();
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
    _reset_fs();
    #
    #compact thrice (this will move the start 'before the horn)
    DevoFS::compact();
    DevoFS::compact();
    DevoFS::compact();
    my $ptr = DevoFS::_get_next_write_addr();
    ok($ptr > 65536 - 4096, msg("Write pointer is in the last sector"));
    #Write mod file (this will go around the horn
    my $len = 0;
    DevoFS::open("protocol/devo.mod", O_CREAT);
    is(DevoFS::write($files{"protocol/devo.mod"}{DATA}, 4096, $len), 0, msg("Wrote mod file"));
    DevoFS::close();
    #
    my $data = "";
    DevoFS::open("protocol/devo.mod", 0);
    is(DevoFS::read($data, 4096, $len), 0, msg("Read written data"));
    _compare_fs(\%files);
}
