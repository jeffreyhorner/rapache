#!perl

use strict;
use warnings FATAL => 'all';
use Apache2::Build;
use constant WIN32 => Apache2::Build::WIN32;

use Cwd;
my $cwd = WIN32 ?
    Win32::GetLongPathName(cwd) : cwd;
$cwd =~ m{^(.+)/glue/perl$} or die "Can't find base cvs directory";
my $base_dir = $1;
my $module_dir = "$base_dir/module";
my $mod_apreq2_dir = Apache2::Build::WIN32 ?
    "$base_dir/win32/libs" : "$module_dir/apache2/.libs";

use base 'Apache::TestRunPerl';

main::->new->run(@ARGV);

sub pre_configure {
    my $self = shift;
    # Don't load an installed mod_apreq
    Apache::TestConfig::autoconfig_skip_module_add('mod_apreq.c');
    Apache::TestConfig::autoconfig_skip_module_add('mod_apreq2.c');
    $self->SUPER::pre_configure();
}

sub configure_modperl {
    my $self = shift;
    my $cfg = $self->{test_config};
    # make sure Win32 uses the just-built libapreq2.dll
    if (WIN32) {
        my $library_dir = $mod_apreq2_dir;
        $cfg->preamble(qq{LoadFile "$library_dir/libapreq2.dll"\n});
    }
    $cfg->preamble(IfModule => '!mod_apreq2.c',
               qq(LoadModule apreq_module "$mod_apreq2_dir/mod_apreq2.so"\n));

    $self->SUPER::configure_modperl();
}
