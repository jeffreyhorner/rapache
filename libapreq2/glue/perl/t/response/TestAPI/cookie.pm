package TestAPI::cookie;
push our @ISA, "APR::Request::Cookie";
use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;

use APR::Request::Cookie;
use APR::Request::Apache2;
use APR::Table;

sub handler {
    my $r = shift;
    plan $r, tests => 30;
    $r->headers_in->{Cookie} = "foo=1;bar=2;foo=3;quux=4";

    my $req = APR::Request::Apache2->handle($r);
    ok defined $req->jar;

    ok t_cmp $req->jar("foo"), 1, "scalar jar(foo)";
    ok t_cmp $req->jar("bar"), 2, "scalar jar(bar)";
    ok t_cmp $req->jar("quux"), 4, "scalar jar(quux)";

    my @rv = $req->jar("foo");
    ok t_cmp "@rv", "1 3", "list jar(foo)";
    @rv = $req->jar("bar");
    ok t_cmp "@rv", "2", "list jar(bar)";
    @rv = $req->jar("quux");
    ok t_cmp "@rv", "4", "list jar(quux)";

    my $jar = $req->jar();
    ok $jar->isa("APR::Request::Cookie::Table");
    ok t_cmp $jar->{foo}, 1, '$jar->{foo} == 1';
    ok t_cmp $jar->{bar}, 2, '$jar->{bar} == 2';
    ok t_cmp $jar->{quux}, 4, '$jar->{quux} == 4';

    my @k = qw/foo bar foo quux/;
    my @v = 1..4;

    ok t_cmp join(" ", keys %$jar), "foo bar foo quux", 'keys %$jar';
    ok t_cmp join(" ", values %$jar), "1 2 3 4", 'values %$jar';

    ok t_cmp join (" ", each %$jar), "$k[$_] $v[$_]", 'each %$jar: ' . $_
        for 0..3;

    ok t_cmp join(" ", $jar->get("foo")), "1 3", '$jar->get("foo")';

    ok not defined $jar->cookie_class("APR::Request::Cookie");
    ok t_cmp $_->is_tainted, 1, "is tainted: $_" for values %$jar;
    $_->is_tainted(0) for values %$jar;
    ok t_cmp $_->is_tainted, 0, "not tainted: $_" for values %$jar;

    eval { $jar->cookie_class("APR::Request::Param") };
    ok t_cmp qr/^Usage/, $@, "Bad class name";

    ok t_cmp $jar->cookie_class(__PACKAGE__), "APR::Request::Cookie", "class upgrade";
    ok $jar->{foo}->isa(__PACKAGE__);

    return 0;
}


1;
