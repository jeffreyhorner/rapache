require 5.006;
use strict;
use warnings;

use Getopt::Long qw(GetOptions);
use Config;

use constant PERL_PATH => $Config{perlpath}; # XXX
use constant WIN32 => ($^O =~ /Win32/);

sub test_prereq {
    system (PERL_PATH, "build/version_check.pl", @_) == 0
        or warn "Please upgrade $_[0] first.\n";
}

test_prereq perl => PERL_PATH;

my %opts;
undef @opts{qw(with-apache2-apxs with-apache1-apxs with-apache2-src
               with-perl with-apache2-httpd
               with-apr-config with-apu-config apxs)};
undef $opts{'with-apache2'} if (WIN32);

my @flags = qw/enable-maintainer-mode enable-perl-glue disable-perl-glue/;
my %args;
# grab from @ARGV only the options that we expect
GetOptions(\%args, (map "$_=s", keys %opts), @flags);

$args{"with-perl"} = PERL_PATH;
my $opts = "";
$opts .= "--enable-maintainer-mode " if $args{"enable-maintainer-mode"};

unless (exists $args{"disable-perl-glue"}) {
    $opts .= "--enable-perl-glue ";
    test_prereq "mod_perl";
    test_prereq "Apache::Test";
    test_prereq "ExtUtils::MakeMaker";
    test_prereq "ExtUtils::XSBuilder";
    test_prereq "Test::More";
}

delete @args{@flags};
$args{"with-apache2-apxs"} = delete $args{apxs}
    if exists $args{apxs} and not exists $args{"with-apache2-apxs"};

$args{"with-perl-opts"} = "@ARGV" if @ARGV;

if (WIN32) {
    require File::Basename;
    unless ($args{'with-apache2'}) {
        my @candidates = qw(with-apache2-apxs with-apache2-httpd
                            with-apr-config with-apu-config);
        foreach my $opt(@candidates) {
            next unless $args{$opt};
            (my $d = File::Basename::dirname($args{$opt})) =~ s{(\\|/)bin}{};
            if (-d $d) {
                delete $args{$opt};
                $args{'with-apache2'} = $d;
                last;
            }
        }
    }
    my @opts = map{qq/--$_="$args{$_}"/} keys %args;
    my @args = (PERL_PATH, 'win32/Configure.pl', @opts);
    print "@args\n";
    system(@args) == 0 or die "system @args failed: $?";
}
else {
    $opts .= join " ", map {qq/--$_="$args{$_}"/} keys %args;

    my $cmd = "./configure $opts";
    print "$cmd\n";
    exec  $cmd;
}
