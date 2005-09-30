use strict;
use warnings FATAL => 'all';

use Apache::Test;

use Apache::TestUtil;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY);

plan tests => 18, have_lwp;

my $location = "/TestApReq__request";
#print GET_BODY $location;

{
    # basic param() test
    my $test  = 'param';
    my $value = '42.5';
    ok t_cmp(GET_BODY("$location?test=$test&value=$value"),
             $value,
             "basic param");
}


for my $test (qw/slurp bb tempname link fh io bad;query=string%%/) {
    # upload a string as a file
    my $value = ('DataUpload' x 10 . "\n") x 1_000;
    my $result = UPLOAD_BODY("$location?test=$test", content => $value); 
    ok t_cmp($result, $value, "basic upload");
    my $i;
    for ($i = 0; $i < length $value; ++$i) {
        last if substr($value,$i,1) ne substr($result,$i,1);
    }

    ok t_cmp($i, length($value), "basic upload length");    
}


{
    my $value = 'DataUpload' x 100;
    my $result = UPLOAD_BODY("$location?test=type", content => $value); 
    ok t_cmp($result, "text/plain", "type");
}

{
    my $value = 'DataUpload' x 100;
    my $result = UPLOAD_BODY("$location?test=hook", content => $value); 
    ok t_cmp($result, $value, "type");
}

{
    my $value = 'DataUpload' x 100;
    my $result = UPLOAD_BODY("$location?test=disable_uploads;foo=bar1;foo=bar2", 
        content => $value); 
    ok t_cmp($result, "ok", "disabled uploads");
}
