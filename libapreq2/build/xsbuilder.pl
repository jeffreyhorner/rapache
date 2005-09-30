#!/usr/bin/perl
#
#  Usage: cd glue/perl; ../../build/xsbuilder.pl
#
#
#  This script is responsible for building the
#
#       glue/perl/xsbuilder/tables
#
#  directory by parsing the header files in src/.

use strict;
use warnings FATAL => 'all';
use File::Basename;
use constant WIN32 => ($^O =~ /Win32/i);
use Cwd;

my $cwd = WIN32 ?
    Win32::GetLongPathName(cwd) : cwd;

$cwd =~ m{^(.+)/glue/perl$} or die "Can't find base directory";
my $base_dir = $1;
my $inc_dir = "$base_dir/include";
my $mod_dir = "$base_dir/module";
my $xs_dir = "$base_dir/glue/perl/xsbuilder";

sub slurp($$)
{
    open my $file, $_[1] or die "Can't open $_[1]: $!";
    read $file, $_[0], -s $file;
}

my %c_macro_cache = (
                     XS => sub {s/XS\s*\(([^)]+)\)/void $1()/g},
                    );
sub c_macro
{
    return $c_macro_cache{"@_"} if exists $c_macro_cache{"@_"};

    my ($name, $header) = @_;
    my $src;
    if (defined $header) {
        slurp local $_ => "$inc_dir/$header";
        /^#define $name\s*\(([^)]+)\)\s+(.+?[^\\])$/ms
            or die "Can't find definition for '$name': $_";
        my $def = $2;
        my @args = split /\s*,\s*/, $1;
        for (1..@args) {
            $def =~ s/\b$args[$_-1]\b/ \$$_ /g;
        }
        my $args = join ',' => ('([^,)]+)') x @args;
        $src = "sub { /^#define $name.+?[^\\\\]\$/gms +
                      s{$name\\s*\\($args\\)}{$def}g}";
    }
    else {
        $src = "sub { /^#define $name.+?[^\\\\]\$/gms +
                      s{$name\\s*\\(([^)]+)\\)}{\$1}g}";
    }
    return $c_macro_cache{"@_"} = eval $src;
}



package My::ParseSource;
use constant WIN32 => ($^O =~ /Win32/i);
my @dirs = ("$inc_dir", "$mod_dir/apache2/", "$base_dir/glue/perl/xsbuilder");
use base qw/ExtUtils::XSBuilder::ParseSource/;


__PACKAGE__->run;

system("touch $base_dir/glue/perl/xsbuilder") == 0
    or die "touch $base_dir/glue/perl/xsbuilder failed: $!"
    unless WIN32;


sub package {'APR::Request'}
sub unwanted_includes {[qw/apreq_config.h apreq_private_apache2.h/]}

# ParseSource.pm v 0.23 bug: line 214 should read
# my @dirs = @{$self->include_dirs};
# for now, we override it here just to work around the bug

sub find_includes {
    my $self = shift;
    return $self->{includes} if $self->{includes};
    require File::Find;
    my(@dirs) = @{$self->include_dirs};
    unless (-d $dirs[0]) {
        die "could not find include directory";
    }
    # print "Will search @dirs for include files...\n" if ($verbose) ;
    my @includes;
    my $unwanted = join '|', @{$self -> unwanted_includes} ;

    for my $dir (@dirs) {
        File::Find::finddepth({
                               wanted => sub {
                                   return unless /\.h$/;
                                   return if ($unwanted && (/^($unwanted)/o));
                                   my $dir = $File::Find::dir;
                                   push @includes, "$dir/$_";
                               },
                               follow => not WIN32,
                              }, $dir);
    }
    return $self->{includes} = $self -> sort_includes (\@includes) ;
}

sub include_dirs {\@dirs}

sub preprocess
{
    # need to macro-expand APREQ_DECLARE et.al. so P::RD can DTRT with
    # ExtUtils::XSBuilder::C::grammar

    for ($_[1]) {
        ::c_macro("APREQ_DECLARE", "apreq.h")->();
        ::c_macro("APREQ_DECLARE_HOOK", "apreq_parser.h")->();
        ::c_macro("APREQ_DECLARE_PARSER", "apreq_parser.h")->();
        ::c_macro("APR_DECLARE")->();
        ::c_macro("XS")-> ();
        s/APR_INLINE//g;
        s/static//g;
    }
}

1;
