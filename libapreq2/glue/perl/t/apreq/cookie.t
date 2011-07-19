use strict;
use warnings FATAL => 'all';

use Apache::Test;

use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY GET_HEAD);

plan tests => 7, have_lwp;#under_construction; # have_lwp

require HTTP::Cookies;

my $location = "/TestApReq__cookie";

{
    my $test  = 'new';
    my $value = 'bar';
    ok t_cmp(GET_BODY("$location?test=new"),
             $value,
             $test);
}
{
    my $test  = 'new';
    my $value = 'bar';
    ok t_cmp(GET_BODY("$location?test=new;expires=%2B3M"),
             $value,
             $test);
}
{
    my $test  = 'netscape';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{$key=$value};
    ok t_cmp(GET_BODY("$location?test=$test&key=$key", Cookie => $cookie),
             $value,
             $test);
}
{
    my $test  = 'rfc';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{\$Version="1"; $key="$value"; \$Path="$location"};
    ok t_cmp(GET_BODY("$location?test=$test&key=$key", Cookie => $cookie),
             qq{"$value"},
             $test);
}
{
    my $test  = 'encoded value with space';
    my $key   = 'apache';
    my $value = 'okie dokie';
    my $cookie = "$key=" . join '',
        map {/ / ? '+' : sprintf '%%%.2X', ord} split //, $value;
    ok t_cmp(GET_BODY("$location?test=$test&key=$key", Cookie => $cookie),
             $value,
             $test);
}
{
    my $test  = 'bake';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = "$key=$value";
    my ($header) = GET_HEAD("$location?test=$test&key=$key", 
                            Cookie => $cookie) =~ /^#Set-Cookie:\s+(.+)/m;

    ok t_cmp($header, $cookie, $test);
}
{
    my $test  = 'bake2';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{\$Version="1"; $key="$value"; \$Path="$location"};
    my ($header) = GET_HEAD("$location?test=$test&key=$key", 
                            Cookie => $cookie) =~ /^#Set-Cookie2:\s+(.+)/m;
    ok t_cmp($header, qq{$key="$value"; Version=1; path="$location"}, $test);
}
