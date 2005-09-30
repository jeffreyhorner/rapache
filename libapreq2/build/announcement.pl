#! /usr/bin/perl
use warnings;
use strict;

our($PACKAGE_NAME, $PACKAGE_TITLE, $PACKAGE_VERSION, $LIBRARY_VERSION);
foreach (qw/PACKAGE_TITLE PACKAGE_NAME PACKAGE_VERSION LIBRARY_VERSION/) {
    no strict 'refs';
    my $opt = lc;
    $opt =~ tr/_/-/;
    chomp ($$_ = `./apreq2-config --$opt`);
}
sub slurp {
    my $file_name = shift;
    open my $fh, "<", $file_name or die "Can't open $file_name: $!";
    read $fh, my $buf, -s $fh;
    return wantarray ? split /\n/, $buf : $buf;
}

my $MAIL_FROM = shift;
my $RCPT_TO = join ",\n      ", map "<$_>" ,
    qw(
         announce@httpd.apache.org
        apreq-dev@httpd.apache.org
         announce@perl.apache.org
          modperl@perl.apache.org
      );

my ($LICENSE_VERSION) = grep s/^\s+Version (\d\.\d),.*$/$1/, slurp "LICENSE";
my $CPAN_DATA = join "", <>;

$CPAN_DATA =~ /^has entered CPAN as(.+?)\nNo action is required/ms
    or die "Bad CPAN message:\n$CPAN_DATA";
$CPAN_DATA = $1;

my $TITLE = "$PACKAGE_TITLE Released";

my $mail_header = <<EOH;
Subject: [ANNOUNCE] $TITLE
From: $MAIL_FROM
To:   $RCPT_TO
EOH

my $preamble = <<EOT;
        $TITLE

The Apache Software Foundation and The Apache HTTP Server Project
are pleased to announce the $PACKAGE_VERSION release of libapreq2.  This
Announcement notes significant changes introduced by this release.

libapreq2-$PACKAGE_VERSION is released under the Apache License
version $LICENSE_VERSION.  It is now available through the ASF mirrors

      http://httpd.apache.org/apreq/download.cgi
      http://httpd.apache.org/apreq/download.cgi

and has entered the CPAN as $CPAN_DATA
EOT


my $changes = (split /\s+[@]section\s+v\S+\s+/, slurp "CHANGES")[1];

print <<EOM;
$mail_header

$preamble
libapreq2 is an APR-based shared library used for parsing HTTP cookies,
query-strings and POST data.  This package provides

    1) version $LIBRARY_VERSION of the libapreq2 library,

    2) mod_apreq2, a filter module necessary for using libapreq2
       within the Apache HTTP Server,

    3) the Apache2::Request, Apache2::Cookie, and Apache2::Upload
       perl modules for using libapreq2 with mod_perl2.

========================================================================

$changes
EOM
1;
