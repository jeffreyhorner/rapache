require APR::Request;
use APR::Pool;
push @ISA, "APR::Request";

sub upload {
    my $req = shift;
    my $body = $req->body;
    $body->param_class("APR::Request::CGI::Upload");
    if (@_) {
        return grep {$_->upload} $body->get(shift) if wantarray;
        for ($body->get(shift)) {
            return $_ if $_->upload;
        }
    }
    return map {$_->upload ? $_->name : () } values %$body;
}


package APR::Request::CGI::Upload;
use APR::Request::Param;
push our @ISA, "APR::Request::Param";

sub type {}
sub filename {}
sub link {}
sub fh {}
sub tempname {}
sub io {}
sub slurp {}
