package TestApReq::upload;

use strict;
use warnings FATAL => 'all';

use Apache2::RequestRec;
use Apache2::RequestIO;
use Apache2::Request ();
use Apache2::Upload;
use File::Spec;
require File::Basename;

sub handler {
    my $r = shift;
    my $req = Apache2::Request->new($r);
    my $temp_dir = File::Spec->tmpdir;

    my $method  = $req->APR::Request::args('method');
    my $has_md5  = $req->APR::Request::args('has_md5');
    require Digest::MD5 if $has_md5;
    my $upload = $req->upload(($req->upload)[0]);
    my $type = $upload->type;
    my $basename = File::Basename::basename($upload->filename);
    my ($data, $fh);

    if ($method eq 'slurp') {
        $upload->slurp($data);
    }
    elsif ($method eq 'fh') {
        read $upload->fh, $data, $upload->size;
    }
    elsif ($method eq 'tempname') {
        my $name = $upload->tempname;
        open $fh, "<", $name or die "Can't open $name: $!";
        binmode $fh;
        read $fh, $data, $upload->size;
        close $fh;
    }
    elsif ($method eq 'link') {
        my $link_file = File::Spec->catfile($temp_dir, "linkfile");
        unlink $link_file if -f $link_file;
        $upload->link($link_file) or die "Can't link to $link_file: $!";
        open $fh, "<", $link_file or die "Can't open $link_file: $!";
        binmode $fh;
        read $fh, $data, $upload->size;
        close $fh;
        unlink $link_file if -f $link_file;
    }
    elsif ($method eq 'io') {
        read $upload->io, $data, $upload->size;
    }
    else  {
        die "unknown method: $method";
    }

    my $temp_file = File::Spec->catfile($temp_dir, $basename);
    unlink $temp_file if -f $temp_file;
    open my $wfh, ">", $temp_file or die "Can't open $temp_file: $!";
    binmode $wfh;
    print $wfh $data;
    close $wfh;
    my $cs = $has_md5 ? cs($temp_file) : 0;
 
    $req->content_type('text/plain');
    my $size = -s $temp_file;
    $r->print(<<END);

type: $type
size: $size
filename: $basename
md5: $cs
END
    unlink $temp_file if -f $temp_file;
    return 0;
}

sub cs {
    my $file = shift;
    open my $fh, '<', $file or die qq{Cannot open "$file": $!};
    binmode $fh;
    my $md5 = Digest::MD5->new->addfile($fh)->hexdigest;
    close $fh;
    return $md5;
}

1;
__END__
