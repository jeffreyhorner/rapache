use strict;
use warnings FATAL => 'all';

use Apache::Test;

use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY);

plan tests => 4, have_lwp;
my $location = "/TestApReq__inherit";
my @response = split/\r?\n/, GET_BODY($location, Cookie=>"apache=2");
ok t_cmp($response[0], "method => GET", "inherit method");
ok t_cmp($response[1], "cookie => apache=2", "inherit cookie");
ok t_cmp($response[2], "DESTROYING TestApReq::inherit object",
         "first object cleanup");
ok t_cmp($response[3], "DESTROYING TestApReq::inherit object",
         "second object cleanup");
