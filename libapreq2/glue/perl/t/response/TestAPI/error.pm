package TestAPI::error;

use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;

use APR::Request::Apache2;
use APR::Request::Error qw/GENERAL TAINTED/;

sub handler {
    my $r = shift;
    plan $r, tests => 3;

    my $req = APR::Request::Apache2->handle($r);
    ok $req->isa("APR::Request");

    # XXX export some constants, and test apreq_xs_strerror
    ok TAINTED > GENERAL;
    ok GENERAL eq "Internal apreq error";
    return 0;
}


1;
