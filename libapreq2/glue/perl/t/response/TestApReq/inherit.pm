package TestApReq::inherit;
use Apache2::Cookie;
use base qw/Apache2::Request Apache2::Cookie::Jar/;
use strict;
use warnings FATAL => 'all';
use APR;
use Apache2::RequestRec;
use Apache2::RequestIO;

sub handler {
    my $r = shift;
    $r = __PACKAGE__->new($r); # tickles refcnt bug in apreq-1
    die "Wrong package: ", ref $r unless $r->isa('TestApReq::inherit');
    $r->content_type('text/plain');
    # look for segfault when $r->isa("Apache2::Request")

    my $req = bless { r => $r };
    $req->printf("method => %s\n", $req->method);
    $req->printf("cookie => %s\n", $req->cookies->{"apache"}->as_string);
    return 0;
}

sub DESTROY { $_[0]->print("DESTROYING ", __PACKAGE__, " object\n") }

1;
