package TestApReq::big_input;

use strict;
use warnings FATAL => 'all';
use Apache2::Request ();
use Apache2::RequestIO;
use Apache2::RequestRec;

sub handler {
    my $r = shift;
    my $req = Apache2::Request->new($r);
    my $len = 0;

    for ($req->param) {
        my $val = $req->param($_) || '';
        $len += length($_) + length($val) + 2; # +2 ('=' and '&')
    }
    $len--; # the stick with two ends one '&' char off

    $req->content_type('text/plain');
    $req->print($len);

    return 0;
}

1;

__END__
