package TestApReq::request;

use strict;
use warnings FATAL => 'all';

use Apache2::RequestRec;
use Apache2::RequestIO;
use Apache2::Request ();
use Apache2::Connection;
use Apache2::Upload;
use APR::Pool;
use APR::Bucket;
use APR::PerlIO;
use Apache2::ServerUtil;
use File::Spec;

my $data;

sub hook {
    my ($upload, $buffer, $len) = @_;
    warn "$upload saw EOS" and return unless defined $buffer;

    die "BAD UPLOAD ARGS" unless length $buffer == $len;
    warn "$upload saw $buffer";
    $data .= $buffer;
}

sub handler {
    my $r = shift;
    my $temp_dir =
        File::Spec->catfile(Apache2::ServerUtil::server_root, 'logs'); 
    my $req = Apache2::Request->new($r);#, POST_MAX => 1_000_000,
                                        #TEMP_DIR => $temp_dir);
    $req->temp_dir($temp_dir);
    $req->read_limit(1_000_000);
    $req->content_type('text/plain');

    my $test  = $req->APR::Request::args('test');
    my $method = $r->method;

    if ($test eq 'param') {
        my $table = $req->APR::Request::args();
        my $value = $req->param('value');
        $req->print($value);
    }
    elsif ($test eq 'slurp') {
        my ($upload) = values %{$req->upload};
        $upload->slurp(my $data);
        if ($upload->size != length $data) {
            $req->print("Size mismatch: size() reports ", $upload->size,
                        " but slurp() length is ", length $data, "\n");
        }
        $req->print($data);
    }
    elsif ($test eq 'bb') {
        my ($upload) = $req->upload("HTTPUPLOAD");
        my $bb = $upload->bb;
        my $e = $bb->first;
        while ($e) {
            $e->read(my $buf);
            $req->print($buf);
            $e = $bb->next($e);
        }
    }
    elsif ($test eq 'tempname') {
        my $upload = $req->upload("HTTPUPLOAD");
        my $name = $upload->tempname;
        my ($dir) = $name =~ /^(.+)apreq\w{6}$/;
        chop $dir;
        die "Tempfile in wrong temp_dir (expected $temp_dir, saw $dir)" unless
            $dir eq $temp_dir;
        open my $fh, "<", $name or die "Can't open $name: $!";
        $r->print(<$fh>);
    }
    elsif ($test eq 'link') {
        my $upload = $req->upload("HTTPUPLOAD");
        my $link_file = File::Spec->catfile("$temp_dir", "linktest");
        unlink $link_file if -f $link_file;
        $upload->link($link_file) or die "Can't link to $link_file: $!";
        open my $fh, "<", $link_file or die "Can't open $link_file: $!";
        $r->print(<$fh>);
    }
    elsif ($test eq 'fh') {
        my $upload = $req->upload(($req->upload)[0]);
        my $fh = $upload->fh;
        read $upload->fh, my $fh_contents, $upload->size;
        $upload->slurp(my $slurp_data);
        die 'fh contents != slurp data'
            unless $fh_contents eq $slurp_data;
        read $fh, $fh_contents, $upload->size;
        die '$fh contents != slurp data'
            unless $fh_contents eq $slurp_data;
        seek $fh, 0, 0;
        $r->print(<$fh>);
    }
    elsif ($test eq 'io') {
        my $upload = $req->upload(($req->upload)[0]);
        my $io = $upload->io;
        read $upload->io, my $io_contents, $upload->size;
        $upload->slurp(my $slurp_data);
        die "io contents != slurp data" unless $io_contents eq $slurp_data;
        undef $io_contents;
        $upload->io->read($io_contents, $upload->size);
        die "io contents != slurp data" unless $io_contents eq $slurp_data;

        my $bb = $upload->bb;
        my $e = $bb->first;
        my $bb_contents = "";
        while ($e) {
            $e->read(my $buf);
            $bb_contents .= $buf;
            $e = $bb->next($e);
        }
        die "io contents != brigade contents"
            unless $io_contents eq $bb_contents;
        $r->print(<$io>);
    }
    elsif ($test eq 'bad') {
        require APR::Request::Error;
        eval {my $q = $req->APR::Request::args('query')};
        if (ref $@ && $@->isa("APR::Request::Error")) {
            $req->upload("HTTPUPLOAD")->slurp(my $data);
            $req->print($data);
        }
    }
    elsif ($test eq 'hook') {
        $data = "";
        $req->upload_hook(\&hook);
        $req->parse;
        $r->print($data);
    }
    elsif ($test eq 'type') {
        my $upload = $req->upload("HTTPUPLOAD");
        die "content-type mismatch"
            unless $upload->info->{"Content-Type"} eq $upload->type;
        $r->print($upload->type);
    }
    elsif ($test eq 'disable_uploads') {
        require APR::Request::Error;
        $req->disable_uploads(1);
        eval {my $upload = $req->upload('HTTPUPLOAD')};
        if (ref $@ eq "APR::Request::Error") {
            my $args = $@->{_r}->APR::Request::args('test'); # checks _r is an object ref
            my $upload = $@->body('HTTPUPLOAD'); # no exception this time!
            die "args test failed" unless $args eq $test;
            $args = $@->APR::Request::args;
            my $test_string = "";

            # MAGIC ITERATOR TESTS
            if ($^V ge v5.8.0) {
                warn "Running MAGIC ITERATOR tests";
                $test_string .= "$a=$b;" while ($a, $b) = each %$args;
                die "each test failed: '$test_string'" unless
                    $test_string eq "test=disable_uploads;foo=bar1;foo=bar2;";

                $test_string = join ":", values %$args;
                die "values test failed: '$test_string'" unless
                    $test_string eq "disable_uploads:bar1:bar2";

                $test_string = join ":", %$args;
                die "list deref test failed: '$test_string'" unless
                    $test_string eq "test:disable_uploads:foo:bar1:foo:bar2";
            }

            # TABLE DO TESTS
            {
                my $do_data = "";
                $args->do( sub { $do_data .= "@_"; 1 } );
                die "do() test failed: '$do_data'" unless
                    $do_data eq "test disable_uploadsfoo bar1foo bar2";
            }


            $req->print("ok");
        }
    }

    return 0;
}
1;
__END__
