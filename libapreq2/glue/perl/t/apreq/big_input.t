use strict;
use warnings FATAL => 'all';

# test the processing of variations of the key lengths and the keys
# numbers
 
use Apache::Test;
use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY POST_BODY);

my $location = "/TestApReq__big_input";

my @key_len = (5, 100, 305);
my @key_num = (5, 15, 26);
my @keys    = ('a'..'z');

my @big_key_len = (100, 500, 5000, 10000);
my @big_key_num = (5, 15, 25);
my @big_keys    = ('a'..'z');

plan tests => @key_len * @key_num + @big_key_len * @big_key_num, have_lwp;


# GET
my $len = 0;
for my $key_len (@key_len) {
    for my $key_num (@key_num) {
        my @query = ();

        for my $key (@keys[0..($key_num-1)]) {
            my $pair = "$key=" . 'd' x $key_len;
            $len += length $pair;
            push @query, $pair;
        }
        my $query = join "&", @query;
        $len += @query - 1;  # the stick with two ends one '&' char off

        my $body = GET_BODY "$location?$query";
        t_debug "# of keys : $key_num, key_len $key_len";
        ok t_cmp($body,
                 ( ($key_len + 3) * $key_num - 1),
                 "GET long query");
    }

}

# POST
$len = 0;
for my $big_key_len (@big_key_len) {
    for my $big_key_num (@big_key_num) {
        my @query = ();

        for my $big_key (@big_keys[0..($big_key_num-1)]) {
            my $pair = "$big_key=" . 'd' x $big_key_len;
            $len += length $pair;
            push @query, $pair;
        }
        my $query = join "&", @query;
        $len += @query - 1;  # the stick with two ends one '&' char off

        my $body = POST_BODY $location, content => $query;
        t_debug "# of keys : $big_key_num, key_len $big_key_len";
        ok t_cmp($body,
                 ( ($big_key_len + 3) * $big_key_num - 1),
                 "POST big data");
    }

}
