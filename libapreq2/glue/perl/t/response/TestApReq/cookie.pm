package TestApReq::cookie;

use strict;
use warnings FATAL => 'all';

use Apache2::RequestIO;
use Apache2::RequestRec;
use Apache2::Connection;

use Apache2::Cookie ();
use Apache2::Request ();

sub handler {
    my $r = shift;
    my $req = Apache2::Request->new($r);
    my %cookies = Apache2::Cookie->fetch($r);

    $r->content_type('text/plain');
    my $test = $req->APR::Request::args('test');
    my $key  = $req->APR::Request::args('key');

    if ($key and $cookies{$key}) {
        if ($test eq "bake") {
            $cookies{$key}->bake($r);
        }
        elsif ($test eq "bake2") {
            $cookies{$key}->bake2($r);
        }
        $r->print($cookies{$key}->value);
    }
    else {
        my @expires;
        @expires = ("expires", $req->APR::Request::args('expires'))
            if $req->APR::Request::args('expires');
        my $cookie = Apache2::Cookie->new($r, name => "foo",
                                             value => "bar", @expires);
        if ($test eq "bake") {
            $cookie->bake($req);
        }
        elsif ($test eq "bake2") {
            $cookie->set_attr(version => 1);
            $cookie->bake2;
        }
        $r->print($cookie->value);
    }


    return 0;
}

1;

__END__
