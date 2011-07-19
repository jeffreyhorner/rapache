use APR::Request;
use APR::Table;
use APR::Brigade;

sub upload_io {
    tie local (*FH), "APR::Request::Brigade", shift->upload;
    return bless *FH{IO}, "APR::Request::Brigade::IO";
}

sub upload_fh {
    my $fname = shift->upload_tempname(@_);
    open my $fh, "<", $fname
        or die "Can't open ", $fname, ": ", $!;
    binmode $fh;
    return $fh;
}

sub APR::Request::upload {
    my $req = shift;
    my $body = $req->body or return;
    $body->param_class(__PACKAGE__);
    if (@_) {
        my @uploads = grep $_->upload, $body->get(@_);
        return wantarray ? @uploads : $uploads[0];
    }

    return map { $_->upload ? $_->name : () } values %$body
        if wantarray;

   return $body->uploads($req->pool);

}

package APR::Request::Brigade;
push our(@ISA), "APR::Brigade";

package APR::Request::Brigade::IO;
push our(@ISA), ();
