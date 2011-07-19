package TestAPI::param;
push our @ISA, "APR::Request::Param";

use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;

use APR::Request::Param;
use APR::Request::Apache2;


sub handler {
    my $r = shift;
    plan $r, tests => 30;
    $r->args("foo=1;bar=2;foo=3;quux=4");

    my $req = APR::Request::Apache2->handle($r);
    ok defined $req->args;
    ok t_cmp $req->args("foo"), 1, "scalar args(foo)";
    ok t_cmp $req->args("bar"), 2, "scalar args(bar)";
    ok t_cmp $req->args("quux"), 4, "scalar args(quux)";

    my @rv = $req->args("foo");
    ok t_cmp "@rv", "1 3", "list args(foo)";
    @rv = $req->args("bar");
    ok t_cmp "@rv", "2", "list args(bar)";
    @rv = $req->args("quux");
    ok t_cmp "@rv", "4", "list args(quux)";

    my $args = $req->args();
    ok $args->isa("APR::Request::Param::Table");
    ok t_cmp $args->{foo}, 1, '$args->{foo} == 1';
    ok t_cmp $args->{bar}, 2, '$args->{bar} == 2';
    ok t_cmp $args->{quux}, 4, '$args->{quux} == 4';

    my @k = qw/foo bar foo quux/;
    my @v = 1..4;

    ok t_cmp join(" ", keys %$args), "foo bar foo quux", 'keys %$args';
    ok t_cmp join(" ", values %$args), "1 2 3 4", 'values %$args';

    ok t_cmp join (" ", each %$args), "$k[$_] $v[$_]", 'each %$args: ' . $_
        for 0..3;

    ok t_cmp join(" ", $args->get("foo")), "1 3", '$args->get("foo")';

    ok not defined $args->param_class("APR::Request::Param");
    ok t_cmp $_->is_tainted, 1, "is tainted: $_" for values %$args;
    $_->is_tainted(0) for values %$args;
    ok t_cmp $_->is_tainted, 0, "not tainted: $_" for values %$args;


    eval { $args->param_class("APR::Request::Cookie") };
    ok t_cmp qr/^Usage/, $@, "Bad class name";

    ok t_cmp $args->param_class(__PACKAGE__), "APR::Request::Param", "class upgrade";
    ok $args->{foo}->isa(__PACKAGE__);

    return 0;
}


1;
