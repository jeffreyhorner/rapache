use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;
use Apache::TestRequest qw(UPLOAD_BODY GET_BODY_ASSERT);
use Cwd;
require File::Basename;

my $cwd = getcwd();
my $location = "/TestApReq__upload";

my %types = (perl => 'application/octet-stream',
             httpd => 'application/octet-stream',
             'perltoc.pod' => 'text/x-pod',
             'perlport.pod' => 'text/x-pod');
my @names = sort keys %types;
my @methods = sort qw/slurp fh tempname link io/;

plan tests => @names * @methods, have_lwp;

foreach my $name (@names) {
    my $url = ( ($name =~ /\.pod$/) ?
        "getfiles-perl-pod/" : "/getfiles-binary-" ) . $name;
    my $content = GET_BODY_ASSERT($url);
    my $path = File::Spec->catfile($cwd, 't', $name);
    open my $fh, ">", $path or die "Cannot open $path: $!";
    binmode $fh;
    print $fh $content;
    close $fh;
}

eval {require Digest::MD5;};
my $has_md5 = $@ ? 0 : 1;

foreach my $file( map {File::Spec->catfile($cwd, 't', $_)} @names) {
    my $size = -s $file;
    my $cs = $has_md5 ? cs($file) : 0;
    my $basename = File::Basename::basename($file);

    for my $method ( @methods) {
        my $result = UPLOAD_BODY("$location?method=$method;has_md5=$has_md5", 
                                 filename => $file);
        my $expected = <<END;

type: $types{$basename}
size: $size
filename: $basename
md5: $cs
END
        ok t_cmp($result, $expected, "$method test for $basename");
    }
    unlink $file if -f $file;
}

sub cs {
    my $file = shift;
    open my $fh, '<', $file or die qq{Cannot open "$file": $!};
    binmode $fh;
    my $md5 = Digest::MD5->new->addfile($fh)->hexdigest;
    close $fh;
    return $md5;
}
