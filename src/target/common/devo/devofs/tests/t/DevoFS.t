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
use Test::More tests => 1;
BEGIN { use_ok('DevoFS') };

#########################

my $fat;
my @imgfiles;
my $tmpdir = File::Temp::tempdir(CLEANUP => 1);
my $imgdir = $FindBin::Bin . "/../image";
my $MAX_FILE_SIZE = 65535;

#tests here
init();
verify_files();

sub init {
    @imgfiles = split(/\n/, `chdir $imgdir; find . -type f`);
    foreach (@imgfiles) {
        s/^\.\///;
    }
    @imgfiles = sort(@imgfiles);
    my $image = $tmpdir . "/devofs.img";
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
            $files{$path ? "$path/$fi[0]" : $fi[0]} = $fi[1];
        }
    }
    return %files;
}

sub verify_files {
    my %files = _read_all_files("");
    my @dirs = sort(keys %files);
    ok(eq_array(\@dirs, \@imgfiles), "Matched file list");
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
        my $ref_md5 = Digest::MD5::md5($data);
        is(DevoFS::open($file, 0), 0, "Opened $file");
        my $data1 = "";
        my $ret = DevoFS::read($data1, $MAX_FILE_SIZE, $len);
        is($ret, 0, "Read $file");
        is($len, $files{$file}, "Read expected number of bytes from $file");
        ok(\$data1, "Read $file");
        my $new_md5 = Digest::MD5::md5($data1);
        ok($ref_md5 eq $new_md5, "Data matches reference");
    }
}
