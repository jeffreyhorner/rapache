use strict;
use ModPerl::MM;

use 5.005;

use Apache::Test5005compat;

use Apache::TestMM qw(test clean);
use Apache::TestReport ();
use Apache::TestSmoke ();
use Apache::TestRun ();
use Apache::TestConfigPerl ();
use Apache::TestSmokePerl ();
use Apache::TestReportPerl ();

use Config;
use File::Find qw(finddepth);
use File::Basename;
use Apache2::Build;
use constant WIN32 => Apache2::Build::WIN32;
use Cwd;
use ExtUtils::XSBuilder::ParseSource;

my $version = "2.XX-dev"; # DUMMY VALUE

my $cwd = WIN32 ?
    Win32::GetLongPathName(cwd) : cwd;
$cwd =~ m{^(.+)/glue/perl$} or die "Can't find base directory";
my $base_dir = $1;
my $inc_dir = "$base_dir/include";
my $lib_dir = "$base_dir/library";
my $xs_dir = "$base_dir/glue/perl/xsbuilder";

sub slurp($$)
{
    open my $file, $_[1] or die "Can't open $_[1]: $!";
    read $file, $_[0], -s $file;
}

my ($apache_includes, $apache_dir, $apr_libs, $apreq_libname, $perl_lib);

if (WIN32) {
    # XXX May need fixing, Randy!
    slurp my $config => "$base_dir/configure.ac";
    $config =~ /^AC_INIT[^,]+,\s*([^,\s]+)/m or 
        die "Can't find version string";
    $version = $1;
    slurp my $make => "$base_dir/Makefile";
    $make =~ /^APACHE=(\S+)/m or
        die "Cannot find top-level Apache directory";
    ($apache_dir = $1) =~ s!\\!/!g;
    ($apache_includes = "-I$apache_dir" . '/include') =~ s!\\!/!g;
    ($apr_libs = "-L$apache_dir" . '/lib') =~ s!\\!/!g;
    $make =~ /^APR_LIB=(\S+)/m or
        die "Cannot find apr lib";
    $apr_libs .= ' -l' . basename($1, '.lib');
    $make =~ /^APU_LIB=(\S+)/m or
        die "Cannot find aprutil lib";
    $apr_libs .= ' -l' . basename($1, '.lib');
    $apreq_libname = 'apreq2';
    $perl_lib = $Config{installsitelib} . '\auto\libaprext';
    $perl_lib =~ s{\\}{\\\\}g;
}
else {
    slurp my $config => "$base_dir/config.status";
    $config =~ /^s,\@APACHE2_INCLUDES\@,([^,]+)/m or
        die "Can't find apache include directory";
    $apache_includes = $1;
    $config =~ /^s,\@APR_INCLUDES\@,([^,]+)/m or
        die "Can't find apache include directory";
    $apache_includes .= " $1";

#    $config =~ m/^s,\@APR_LDFLAGS\@,([^,]+)/m or
#        die "Can't find apr ldflags";
#    $apr_libs = $1;

#    $config =~ m/^s,\@APR_LIBS\@,([^,]+)/m or
#        die "Can't find apr libraries";
#    $apr_libs .= " $1";

    $apr_libs ="";

    $config =~ m/^s,\@APREQ_LIBNAME\@,([^,]+)/m or
        die "Can't find apreq libname";
    $apreq_libname = $1;

    $config =~ m/^s,\@PACKAGE_VERSION\@,([^,]+)/m or
        die "Can't find package version";
    $version = $1;

}

my $apreq_libs = WIN32 ?
    qq{-L$base_dir/win32/libs -llib$apreq_libname -lmod_apreq2 -L$perl_lib -llibaprext -L$apache_dir/lib -lmod_perl} :
    qx{$base_dir/apreq2-config --link-ld --ldflags --libs};

my $mp2_typemaps = Apache2::Build->new->typemaps;

package My::ParseSource;
use base qw/ExtUtils::XSBuilder::ParseSource/;
use constant WIN32 => ($^O =~ /Win32/i);
my @dirs = ("$base_dir/include", "$base_dir/module/apache2");
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

package My::WrapXS;
use base qw/ExtUtils::XSBuilder::WrapXS/;
our $VERSION = $version;
use constant WIN32 => ($^O =~ /Win32/i);

##################################################
# Finally, we get to the actual script...

__PACKAGE__ -> run;

my @scripts = ();

use File::Spec::Functions qw(catfile);

File::Find::finddepth(sub {
    return unless /(.*?\.pl)\.PL$/;
    push @scripts, "$File::Find::dir/$1";
}, '.');

Apache::TestMM::filter_args();
Apache::TestMM::generate_script("t/TEST");
Apache::TestSmokePerl->generate_script;
Apache::TestReportPerl->generate_script;

my %opts = (
    NAME => 'libapreq2',
    DIR => [qw(xs)],
    clean => { FILES => "xs t/logs t/TEST @scripts" },
    realclean => { FILES => "xsbuilder/tables" },
);

ModPerl::MM::WriteMakefile(%opts);

# That's the whole script - below is just a bunch of local overrides
##################################################
sub get_functions {
    my $self = shift;
    $self->{XS}->{"APR::Request::Error"} ||= [];
    $self->SUPER::get_functions;
}

sub copy_docs {
    my @lines;
    my $dfs = '$(DIRFILESEP)';

    return; # XXX FIXME when APR::Request gets documentation

    foreach my $file (@_) {
        my @dirs;
        $file =~ /(\w+\.pod)$/ or next;
        if ($1 eq "Error.pod" or $1 eq "Table.pod") {
            push @dirs, "Apache$dfs$_" for qw/Request Cookie Upload/;
            push @dirs, join $dfs, qw/Apache Cookie Jar/ if $1 eq "Error.pod";
        }
        else {
            push @dirs, "Apache";
        }
        push @lines, map <<EOT, @dirs;
subdirs :: \$(INST_LIBDIR)$dfs$_$dfs$1

\$(INST_LIBDIR)$dfs$_$dfs$1: $file
	\$(NOECHO) \$(MKPATH) \$(INST_LIBDIR)$dfs$_
	\$(CP) $file \$(INST_LIBDIR)$dfs$_$dfs$1

EOT
    }
    return join "", @lines;
}

sub test_docs {
    my ($pods, $tests) = @_;

    my $pod2test = Apache::TestConfig::which('pod2test');
    unless ($pod2test) {
        # try under the perl's bin
        require Config;
        my $bin = $Config::Config{bin};
        $pod2test = catfile $bin, "pod2test";
    }

    return "" unless $pod2test and -e $pod2test;

    return join "", map <<EOT, 0..$#$pods;
$$tests[$_]: $$pods[$_]
	\$(FULLPERLRUN) $pod2test $$pods[$_] $$tests[$_]

EOT
}

sub MY::postamble {
    my @docs = <docs/*.pod>;
    my @tests = @docs;
    s/pod$/t/ for @tests;

    my $string = copy_docs(@docs);

    my $test_docs = test_docs(\@docs, \@tests);

    if ($test_docs) {
        $string .= $test_docs;
        $string .= <<EOT;
doc_test : @tests
	\$(FULLPERLRUN) "-Mblib" "-MTest::Harness" "-e" "runtests(\@ARGV)" @tests

#test :: doc_test

EOT
    } else {
        $string .= <<EOT;
test ::
	\$(NOECHO) \$(ECHO) pod2test was not found, skipping inlined tests

EOT
    }

    return $string;
}


sub parsesource_objects {[My::ParseSource->new]}
sub new_typemap {My::TypeMap->new(shift)}
sub h_filename_prefix {'apreq_xs_'}
sub my_xs_prefix {'apreq_xs_'}
sub xs_include_dir { $xs_dir }

sub mod_xs {
    my($self, $module, $complete) = @_;
    my $dirname = $self->class_dirname($module);
    my @parts = split '::', $module;
    my $mod_xs = "$dirname/$parts[-1].xs";

    for ($self -> xs_include_dir, @{ $self->{glue_dirs} }) {
        my $file = "$_/$mod_xs";
		$mod_xs = $file if $complete;
        return $mod_xs if -e $file;
    }

    undef;
}

sub mod_pm {
    my($self, $module, $complete) = @_;
    my $dirname = $self->class_dirname($module);
    my @parts = split '::', $module;
    my $mod_pm = "$dirname/$parts[-1].pm";

    for ($self -> xs_include_dir, @{ $self->{glue_dirs} }) {
        my $file = "$_/$mod_pm";
		$mod_pm = $file if $complete;
        return $mod_pm if -e $file;
    }

    undef;
}

#inline mod_xs directly, so we can put XS directives there

sub write_xs {
    my($self, $module, $functions) = @_;

    my $fh = $self->open_class_file($module, '.xs');
    print $fh "$self->{noedit_warning_c}\n";

    my @includes = @{ $self->includes };

    if (my $mod_h = $self->mod_h($module)) {
        push @includes, $mod_h;
    }

    for (@includes) {
        print $fh qq{\#include "$_"\n\n};
    }

    if (my $mod_xs = $self->mod_xs($module, 1)) {
        open my $file, $mod_xs or die "can't open $mod_xs: $!";
        print $fh $_ while <$file>;
        print $fh "\n\n";
    }

    my $last_prefix = "";
    my $fmap = $self -> typemap -> {function_map} ;
    my $myprefix = $self -> my_xs_prefix ;

    for my $func (@$functions) {
        my $class = $func->{class};
        if ($class)
            {
            my $prefix = $func->{prefix};
            $last_prefix = $prefix if $prefix;

            if ($func->{name} =~ /^$myprefix/o) {
                #e.g. mpxs_Apache__RequestRec_
                my $class_prefix = $fmap -> class_c_prefix($class);
                if ($func->{name} =~ /$class_prefix/) {
                    $prefix = $fmap -> class_xs_prefix($class);
                }
            }

            $prefix = $prefix ? "  PREFIX = $prefix" : "";
            print $fh "MODULE = $module    PACKAGE = $class $prefix\n\n";
            }

        print $fh $func->{code};
    }

    if (my $destructor = $self->typemap->destructor($last_prefix)) {
        my $arg = $destructor->{argspec}[0];

        print $fh <<EOF;
void
$destructor->{name}($arg)
    $destructor->{class} $arg

EOF
    }

    print $fh "PROTOTYPES: disabled\n\n";
    print $fh "BOOT:\n";
    print $fh $self->boot($module);
    print $fh "    items = items; /* -Wall */\n\n";

    if (my $newxs = $self->{newXS}->{$module}) {
        for my $xs (@$newxs) {
            print $fh qq{   cv = newXS("$xs->[0]", $xs->[1], __FILE__);\n};
            print $fh qq{   GvSHARED_on(CvGV(cv));\n} if ExtUtils::XSBuilder::WrapXS::GvSHARED();
        }
    }

    close $fh;
}



sub mod_pod {
    my($self, $module, $complete) = @_;
    my $dirname = $self->class_dirname($module);
    my @parts = split '::', $module;
    my $mod_pod = "$dirname/$parts[-1].pod";
    for ($self -> xs_incsrc_dir, @{ $self->{glue_dirs} }) {
        my $file = "$_/$mod_pod";
        $mod_pod = $file if $complete;
        print "mod_pod $mod_pod $file $complete\n" ;
        return $mod_pod if -e $file;
    }
    undef;
}

sub write_docs {
    my ($self, $module, $functions) = @_;
    my $podfile = $self->mod_pod($module, 1) or return;
    my $fh = $self->open_class_file($module, '.pod');
    open my $pod, "<", $podfile or die $!;
    while (<$pod>) {
        print $fh $_;
    }
}
sub pm_text {
    my($self, $module, $isa, $code) = @_;

    my $text = <<"EOF";
$self->{noedit_warning_hash}

package $module;
require DynaLoader ;

use strict;
use warnings FATAL => 'all';

use vars qw{\$VERSION \@ISA} ;
$isa
push \@ISA, 'DynaLoader' ;
\$VERSION = '$version';
bootstrap $module \$VERSION ;

$code

1;
__END__
EOF

        return $text;
}
sub makefilepl_text {
    my($self, $class, $deps,$typemap) = @_;

    my @parts = split (/::/, $class) ;
    my $mmargspath = '../' x @parts ;
    $mmargspath .= 'mmargs.pl' ;

    my $txt = qq{
$self->{noedit_warning_hash}
use ModPerl::MM;

local \$MMARGS ;

if (-f '$mmargspath')
    {
    do '$mmargspath' ;
    die \$\@ if (\$\@) ;
    }

\$MMARGS ||= {} ;


ModPerl::MM::WriteMakefile(
    'NAME'      => '$class',
    'VERSION'   => '$version',
    'TYPEMAPS'  => [qw(@$mp2_typemaps $typemap)],
    'INC'       => "-I$base_dir/glue/perl/xs -I$inc_dir -I$xs_dir $apache_includes",
    'LIBS'      => "$apreq_libs $apr_libs",
} ;
$txt .= "'depend'  => $deps,\n" if ($deps) ;
$txt .= qq{    
    \%\$MMARGS,
);

} ;

}

# For now, just copy the typemap file in xsbuilder til we
# can remove ExtUtils::XSBuilder.

sub write_typemap
{
    my $self = shift;

    my $typemap = $self->typemap;
    my $map = $typemap->get;
    my %seen;

    my $fh = $self->open_class_file('', 'typemap');
    print $fh "$self->{noedit_warning_hash}\n";
    open my $tfh, "$xs_dir/typemap" or die $!;
    print $fh $_ while <$tfh>;
}


package My::TypeMap;
use base 'ExtUtils::XSBuilder::TypeMap';

sub null_type {
    my($self, $type) = @_;
    my $t = $self->get->{$type};
    my $class = $t -> {class} ;

    if ($class =~ /APREQ_COOKIE_VERSION/) {
        return 'APREQ_COOKIE_VERSION_DEFAULT';
    }
    else {
        return $self->SUPER::null_type($type);
    }
}

# XXX this needs serious work
sub typemap_code
{
    {
           T_SUBCLASS  => {
                          INPUT => <<'EOT',
    if (SvROK($arg) || !sv_derived_from($arg, \"$Package\"))
        Perl_croak(aTHX_ \"Usage: argument is not a subclass of $Package\");
    $var = SvPV_nolen($arg)
EOT
                           },

        T_APREQ_COOKIE  => {
                            INPUT  => '$var = apreq_xs_sv2cookie(aTHX_ $arg)',
                            perl2c => 'apreq_xs_sv2cookie(aTHX_ sv)',
                            OUTPUT => '$arg = apreq_xs_cookie2sv(aTHX_ $var, class, parent);',
                            c2perl => 'apreq_xs_cookie2sv(aTHX_ ptr, class, parent)',
                           },

        T_APREQ_PARAM   => {
                            INPUT  => '$var = apreq_xs_sv2param(aTHX_ $arg)',
                            perl2c => 'apreq_xs_sv2param(aTHX_ sv)',
                            OUTPUT => '$arg = apreq_xs_param2sv(aTHX_ $var, class, parent);',
                            c2perl => 'apreq_xs_param2sv(aTHX_ ptr, class, parent)',
                           },

         T_APREQ_HANDLE => {
                            INPUT  => '$var = apreq_xs_sv2handle(aTHX_ $arg)',
                            perl2c => 'apreq_xs_sv2handle(aTHX_ sv)',
                            c2perl => 'apreq_xs_handle2sv(aTHX_ ptr, class, parent)',
                            OUTPUT => '$arg = apreq_xs_handle2sv(aTHX_ $var, class, parent);',
                           },

     T_APREQ_HANDLE_CGI => {
                            INPUT  => '$var = apreq_xs_sv2handle(aTHX_ $arg)',
                            OUTPUT => '$arg = apreq_xs_handle2sv(aTHX_ $var, class, SvRV(ST(1)));'
                           },

 T_APREQ_HANDLE_APACHE2 => {
                            INPUT  => '$var = apreq_xs_sv2handle(aTHX_ $arg)',
                            OUTPUT => <<'EOT',
    $arg = apreq_xs_handle2sv(aTHX_ $var, class, SvRV(ST(1)));
    SvMAGIC(SvRV($arg))->mg_ptr = (void *)r;
EOT
                           },

          T_APREQ_ERROR => {
                             INPUT => '$var = (HV *)SvRV($arg)',
                            OUTPUT => '$arg = sv_bless(newRV_noinc((SV*)$var), gv_stashpvn(\"${ntype}\", sizeof(\"${ntype}\") - 1, FALSE);'
                           },

              T_HASHOBJ => {
                            INPUT => <<'EOT', # '$var = modperl_hash_tied_object(aTHX_ \"${ntype}\", $arg)'
    if (sv_derived_from($arg, \"${ntype}\")) {
        if (SVt_PVHV == SvTYPE(SvRV($arg))) {
            SV *hv = SvRV($arg);
            MAGIC *mg;
            if (SvMAGICAL(hv)) {
                if ((mg = mg_find(hv, PERL_MAGIC_tied))) {
                    $var = (void *)MgObjIV(mg);
                }
                else {
                    Perl_warn(aTHX_ \"Not a tied hash: (magic=%c)\", mg);
                    $var = NULL;
                }
            }
            else {
                Perl_warn(aTHX_ \"SV is not tied\");
                $var = NULL;
            }
        }
        else {
            $var = (void *)SvObjIV($arg);
        }
    }
    else {
        Perl_croak(aTHX_
                   \"argument is not a blessed reference \"
                   \"(expecting an %s derived object)\", \"${ntype}\");
    }
EOT

                 OUTPUT => <<'EOT', # '$arg = modperl_hash_tie(aTHX_ \"${ntype}\", $arg, $var);'
  {
    SV *hv = (SV*)newHV();
    SV *rsv = $arg;
    sv_setref_pv(rsv, \"${ntype}\", $var);
    sv_magic(hv, rsv, PERL_MAGIC_tied, Nullch, 0);
    $arg = SvREFCNT_inc(sv_bless(sv_2mortal(newRV_noinc(hv)),
                                 gv_stashpv(\"${ntype}\", TRUE)));
  }
EOT

                           },
    }
}
