use strict;
use warnings FATAL => 'all';

use Apache::Test;
use Apache::TestUtil qw(t_cmp t_debug t_write_perl_script);
use Apache::TestConfig;
use Apache::TestRequest qw(GET_BODY UPLOAD_BODY 
                           GET_BODY_ASSERT POST_BODY GET_RC GET_HEAD);
use constant WIN32 => Apache::TestConfig::WIN32;
use HTTP::Cookies;
use Cwd;
require File::Basename;

my @key_len = (5, 100, 305);
my @key_num = (5, 15, 26);
my @keys    = ('a'..'z');

my $cwd = getcwd();
my %types = (perl => 'application/octet-stream',
             'perltoc.pod' => 'text/x-pod');
my @names = sort keys %types;
my @methods = sort qw/slurp fh tempname link io/;

my $cgi = File::Spec->catfile(Apache::Test::vars('serverroot'),
                              qw(cgi-bin test_cgi.pl));

t_write_perl_script($cgi, <DATA>);

#########################################################
# uncomment the following to test larger keys
my @big_key_len = (100, 500, 5000, 10000);
# if the above is uncommented, comment out the following
#my @big_key_len = (100, 500, 1000, 2500);
#########################################################
my @big_key_num = (5, 15, 25);
my @big_keys    = ('a'..'z');

plan tests => 10 + @key_len * @key_num + @big_key_len * @big_key_num +
  @names * @methods, have_lwp && have_cgi;

my $location = '/cgi-bin';
my $script = $location . '/test_cgi.pl';
my $line_end = WIN32 ? "\r\n" : "\n";
my $filler = "0123456789" x 6400; # < 64K

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


$body = UPLOAD_BODY("$script?foo=1", content => $filler);
ok t_cmp($body, "\tfoo => 1$line_end", 
         "simple upload");


{
    my $test  = 'netscape';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{$key=$value};
    ok t_cmp(GET_BODY("$script?test=$test&key=$key", Cookie => $cookie),
             $value,
             $test);
}
{
    my $test  = 'rfc';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{\$Version="1"; $key="$value"; \$Path="$location"};
    ok t_cmp(GET_BODY("$script?test=$test&key=$key", Cookie => $cookie),
             qq{"$value"},
             $test);
}
{
    my $test  = 'encoded value with space';
    my $key   = 'apache';
    my $value = 'okie dokie';
    my $cookie = "$key=" . join '',
        map {/ / ? '+' : sprintf '%%%.2X', ord} split //, $value;
    ok t_cmp(GET_BODY("$script?test=$test&key=$key", Cookie => $cookie),
             $value,
             $test);
}
{
    my $test  = 'bake';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = "$key=$value";
    my ($header) = GET_HEAD("$script?test=$test&key=$key", 
                            Cookie => $cookie) =~ /^#Set-Cookie:\s+(.+)/m;
    ok t_cmp($header, $cookie, $test);
}
{
    my $test  = 'bake2';
    my $key   = 'apache';
    my $value = 'ok';
    my $cookie = qq{\$Version="1"; $key="$value"; \$Path="$location"};
    my ($header) = GET_HEAD("$script?test=$test&key=$key", 
                            Cookie => $cookie) =~ /^#Set-Cookie2:\s+(.+)/m;
    ok t_cmp($header, qq{$key="$value"; Version=1; path="$location"}, $test);
}

# file upload tests

foreach my $name (@names) {
    my $url = ( ($name =~ /\.pod$/) ?
        "getfiles-perl-pod/" : "/getfiles-binary-" ) . $name;
    my $content = GET_BODY_ASSERT($url);
    my $path = File::Spec->catfile($cwd, 't', $name);
    open my $fh, ">", $path or die "Cannot open $path: $!";
    binmode $fh;
    print $fh $content;
    close $fh;
}

eval {require Digest::MD5;};
my $has_md5 = $@ ? 0 : 1;

foreach my $file( map {File::Spec->catfile($cwd, 't', $_)} @names) {
    my $size = -s $file;
    my $cs = $has_md5 ? cs($file) : 0;
    my $basename = File::Basename::basename($file);

    for my $method ( @methods) {
        my $result = UPLOAD_BODY("$script?method=$method;has_md5=$has_md5", 
                                 filename => $file);
        $result =~ s{\r}{}g;
        my $expected = <<END;

type: $types{$basename}
size: $size
filename: $basename
md5: $cs
END
        ok t_cmp($result, $expected, "$method test for $basename");
    }
    unlink $file if -f $file;
}

sub cs {
    my $file = shift;
    open my $fh, '<', $file or die qq{Cannot open "$file": $!};
    binmode $fh;
    my $md5 = Digest::MD5->new->addfile($fh)->hexdigest;
    close $fh;
    return $md5;
}

__DATA__
use strict;
use File::Basename;
use warnings FATAL => 'all';
use blib;
use APR;
use APR::Pool;
use APR::Request::Param;
use APR::Request::Cookie;
use APR::Request::CGI;
use File::Spec;
require File::Basename;

my $p = APR::Pool->new();

apreq_log("Creating APR::Request::CGI object");
my $req = APR::Request::CGI->handle($p);

my $foo = $req->param("foo");
my $bar = $req->param("bar");

my $test = $req->param("test");
my $key  = $req->param("key");
my $method  = $req->param("method");

if ($foo || $bar) {
    print "Content-Type: text/plain\n\n";
    if ($foo) {
        apreq_log("foo => $foo");
        print "\tfoo => $foo\n";
    }
    if ($bar) {
        apreq_log("bar => $bar");
        print "\tbar => $bar\n";
    }
}
    
elsif ($test && $key) {
    my $jar = $req->jar;
    $jar->cookie_class("APR::Request::Cookie");
    my %cookies = %$jar;
    apreq_log("Fetching cookie $key");
    if ($cookies{$key}) {
        if ($test eq "bake") {
            printf "Set-Cookie: %s\n", $cookies{$key}->as_string;
        }
        elsif ($test eq "bake2") {
            printf "Set-Cookie2: %s\n", $cookies{$key}->as_string;
        }
        print "Content-Type: text/plain\n\n";
        print APR::Request::decode($cookies{$key}->value);
    }
}

elsif ($method) {
    my $temp_dir = File::Spec->tmpdir;
    my $has_md5  = $req->args('has_md5');
    require Digest::MD5 if $has_md5;
    my $body = $req->body;
    $body->param_class("APR::Request::Param");
    my ($param) = values %{$body->uploads($p)};
    my $type = $param->upload_type;
    my $basename = File::Basename::basename($param->upload_filename);
    my ($data, $fh);

    if ($method eq 'slurp') {
        $param->upload_slurp($data);
    }
    elsif ($method eq 'fh') {
        read $param->upload_fh, $data, $param->upload_size;
    }
    elsif ($method eq 'tempname') {
        my $name = $param->upload_tempname;
        open $fh, "<", $name or die "Can't open $name: $!";
        binmode $fh;
        read $fh, $data, $param->upload_size;
        close $fh;
    }
    elsif ($method eq 'link') {
        my $link_file = File::Spec->catfile($temp_dir, "linkfile");
        unlink $link_file if -f $link_file;
        $param->upload_link($link_file) or die "Can't link to $link_file: $!";
        open $fh, "<", $link_file or die "Can't open $link_file: $!";
        binmode $fh;
        read $fh, $data, $param->upload_size;
        close $fh;
        unlink $link_file if -f $link_file;
    }
    elsif ($method eq 'io') {
        read $param->upload_io, $data, $param->upload_size;
    }
    else  {
        die "unknown method: $method";
    }

    my $temp_file = File::Spec->catfile($temp_dir, $basename);
    unlink $temp_file if -f $temp_file;
    open my $wfh, ">", $temp_file or die "Can't open $temp_file: $!";
    binmode $wfh;
    print $wfh $data;
    close $wfh;
    my $cs = $has_md5 ? cs($temp_file) : 0;
 
    my $size = -s $temp_file;
    print <<"END";


type: $type
size: $size
filename: $basename
md5: $cs
END
    unlink $temp_file if -f $temp_file;
}

else {
    my $len = 0;
    print "Content-Type: text/plain\n\n";
    apreq_log("Fetching all parameters");
    for ($req->param) {
        my $param = $req->param($_);
        next unless $param;
        my $length = length($param);
        apreq_log("$_ has a value of length $length");
        $len += length($_) + $length;
    }
    print $len;
}

sub apreq_log {
    my $msg = shift;
    my ($pkg, $file, $line) = caller;
    $file = basename($file);
    print STDERR "$file($line): $msg\n";    
}

sub cs {
    my $file = shift;
    open my $fh, '<', $file or die qq{Cannot open "$file": $!};
    binmode $fh;
    my $md5 = Digest::MD5->new->addfile($fh)->hexdigest;
    close $fh;
    return $md5;
}
