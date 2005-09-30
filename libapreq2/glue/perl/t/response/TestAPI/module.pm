package TestAPI::module;

use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;

use APR::Request::Apache2;

sub handler {
    my $r = shift;
    plan $r, tests => 9;

    my $req = APR::Request::Apache2->handle($r);
    ok $req->isa("APR::Request::Apache2");

    ok t_cmp $req->brigade_limit, 256 * 1024, "default brigade limit is 256K";
    ok $req->brigade_limit(1024);
    ok t_cmp $req->brigade_limit, 1024, "brigade_limit reset to 1K";

    ok $req->read_limit(1024 * 1024);
    ok t_cmp $req->read_limit, 1024 * 1024, "read_limit reset to 1M";

    ok not defined $req->temp_dir;
    ok $req->temp_dir("/tmp");
    ok t_cmp $req->temp_dir, "/tmp", "temp dir reset to /tmp";

    # XXX parse, header_in & header_out tests

    return 0;
}


1;
