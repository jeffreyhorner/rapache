use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil;
use Apache::TestConfig;
use Apache::TestRequest qw(GET_BODY POST_BODY);

my @key_len = (5, 100, 305);
my @key_num = (5, 15, 26);
my @keys    = ('a'..'z');

my @big_key_len = (100, 500, 5000, 10000);
my @big_key_num = (5, 15, 25);
my @big_keys    = ('a'..'z');

my $num_tests = @key_len * @key_num + @big_key_len * @big_key_num;
$num_tests *= 2 if Apache::Test::have_ssl();
plan tests => $num_tests, have_lwp;

my $location = "/apreq_big_request_test";
my $scheme = 'http';

START_TESTS:
Apache::TestRequest::scheme($scheme);

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
        my $body = GET_BODY "$location?$query";
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
        my $body = POST_BODY($location, content => $query);
        ok t_cmp($body,
                 $len,
                 "POST big data");
    }

}

if (Apache::Test::have_ssl() and $scheme ne 'https') {    
    $scheme = 'https';
    goto START_TESTS;
}
