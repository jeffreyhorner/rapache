use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;
use Apache::TestConfig;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY POST_BODY GET_RC GET_HEAD);
use constant WIN32 => Apache::TestConfig::WIN32;

my @key_len = (5, 100, 305);
my @key_num = (5, 15, 26);
my @keys    = ('a'..'z');

my @big_key_len = (100, 500, 5000, 10000);
my @big_key_num = (5, 15, 25);
my @big_keys    = ('a'..'z');

plan tests => 10 + @key_len * @key_num + @big_key_len * @big_key_num, 
    have_lwp && have_cgi;

require HTTP::Cookies;

my $location = '/cgi-bin';
my $script = $location . (WIN32 ? '/test_cgi.exe' : '/test_cgi');
my $line_end = "\n";
my $filler = "0123456789" x 20; # < 64K

# GET
for my $key_len (@key_len) {
    for my $key_num (@key_num) {
        my @query = ();
        my $len = 0;

        for my $key (@keys[0..($key_num-1)]) {
            my $pair = "$key=" . 'd' x $key_len;
            $len += length($pair) - 1;
            push @query, $pair;
        }
        my $query = join ";", @query;

        t_debug "# of keys : $key_num, key_len $key_len";
        my $body = GET_BODY "$script?$query";
        ok t_cmp($body,
                 $len,
                 "GET long query");
    }

}

# POST
for my $big_key_len (@big_key_len) {
    for my $big_key_num (@big_key_num) {
        my @query = ();
        my $len = 0;

        for my $big_key (@big_keys[0..($big_key_num-1)]) {
            my $pair = "$big_key=" . 'd' x $big_key_len;
            $len += length($pair) - 1;
            push @query, $pair;
        }
        my $query = join ";", @query;

        t_debug "# of keys : $big_key_num, big_key_len $big_key_len";
        my $body = POST_BODY($script, content => $query);
        ok t_cmp($body,
                 $len,
                 "POST big data");
    }

}

ok t_cmp(POST_BODY("$script?foo=1", Content => $filler),
         "\tfoo => 1$line_end", "simple post");

ok t_cmp(GET_BODY("$script?foo=%3F&bar=hello+world"),
          "\tfoo => ?$line_end\tbar => hello world$line_end", "simple get");

my $body = POST_BODY($script, content => 
                     "aaa=$filler;foo=1;bar=2;filler=$filler");
ok t_cmp($body, "\tfoo => 1$line_end\tbar => 2$line_end",
         "simple post");

$body = POST_BODY("$script?foo=1", content => 
                  "intro=$filler&bar=2&conclusion=$filler");
ok t_cmp($body, "\tfoo => 1$line_end\tbar => 2$line_end",
         "simple post");

$body = UPLOAD_BODY("$script?foo=0", content => $filler);
ok t_cmp($body, "\tfoo => 0$line_end",
         "simple upload");


{
    my $test  = 'netscape';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{$key=$value};
    ok t_cmp($value,
             GET_BODY("$script?test=$test&key=$key", Cookie => $cookie),
             $test);
}
{
    my $test  = 'rfc';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{\$Version="1"; $key="$value"; \$Path="$location"};
    ok t_cmp(qq{"$value"},
             GET_BODY("$script?test=$test&key=$key", Cookie => $cookie),
             $test);
}
{
    my $test  = 'encoded value with space';
    my $key   = 'apache';
    my $value = 'okie dokie';
    my $cookie = "$key=" . join '',
        map {/ / ? '+' : sprintf '%%%.2X', ord} split //, $value;
    ok t_cmp($value,
             GET_BODY("$script?test=$test&key=$key", Cookie => $cookie),
             $test);
}
{
    my $test  = 'bake';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = "$key=$value";
    my ($header) = GET_HEAD("$script?test=$test&key=$key", 
                            Cookie => $cookie) =~ /^#Set-Cookie:\s+(.+)/m;
    ok t_cmp($cookie, $header, $test);
}
{
    my $test  = 'bake2';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{\$Version="1"; $key="$value"; \$Path="$location"};
    my ($header) = GET_HEAD("$script?test=$test&key=$key", 
                            Cookie => $cookie) =~ /^#Set-Cookie2:\s+(.+)/m;
    ok t_cmp(qq{$key="$value"; Version=1; path="$location"}, $header, $test);
}
