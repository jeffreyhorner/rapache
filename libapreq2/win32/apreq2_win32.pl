#!perl
use strict;
use Config;
use Getopt::Long;
require Win32;
use ExtUtils::MakeMaker;
use File::Spec::Functions qw(catfile catdir);
use warnings;
use FindBin;

BEGIN {
    die 'This script is intended for Win32' unless $^O =~ /Win32/i;
}

my $license = <<'END';
# ====================================================================
#
#  Copyright 2003-2005  The Apache Software Foundation
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

# apreq2 script designed to allow easy command line access to apreq2
# configuration parameters.

END

my $file = 'apreq2-config.pl';
my $apreq_home = Win32::GetShortPathName($FindBin::Bin);
$apreq_home =~ s!/?win32$!!;
$apreq_home =~ s!/!\\!g;
require "$apreq_home/win32/util.pl";

my ($prefix, $help);
GetOptions('with-apache2=s' => \$prefix, 'help' => \$help) or usage($0);
usage($0) if $help;

unless (defined $prefix and -d $prefix) {
    $prefix = prompt("Please give the path to your Apache2 installation:",
		     $prefix);
}
die "Can't find a suitable Apache2 installation!" 
    unless (-d $prefix and check($prefix));

$prefix = Win32::GetShortPathName($prefix);
my %ap_dir;
foreach (qw(bin lib include build)) {
    $ap_dir{$_} = catdir $prefix, $_;
}

my $src_version = catfile $apreq_home, 'include', 'apreq_version.h';
my $apache_version = catfile $ap_dir{include}, 'apreq_version.h';

my $apreq_version = -e $src_version ? $src_version : $apache_version;
open(my $inc, $apreq_version)
    or die "Cannot open $apreq_version: $!";
my %vers;
while (<$inc>) {
    if (/define\s+APREQ_(MAJOR|MINOR|PATCH)_VERSION\s+(\d+)/) {
        $vers{$1} = $2;
    }
}
close $inc;
my $dotted = "$vers{MAJOR}.$vers{MINOR}.$vers{PATCH}";
my $src_dir = -d $apreq_home ? $apreq_home : '';

my %apreq_args = (APREQ_MAJOR_VERSION => $vers{MAJOR},
                  APREQ_DOTTED_VERSION => $dotted,
                  APREQ_LIBNAME => 'libapreq2.lib',
                  prefix => $prefix,
                  exec_prefix => $prefix,
                  bindir => $ap_dir{bin},
                  libdir => $ap_dir{lib},
                  datadir => $prefix,
                  installbuilddir => $ap_dir{build},
                  includedir => $ap_dir{include},
                
                  CC => $Config{cc},
                  CPP => $Config{cpp},
                  LD => $Config{ld},
                  SHELL => $ENV{comspec},
                  CPPFLAGS => '',
                  CFLAGS => q{ /nologo /MD /W3 /O2 /D "WIN32" /D "_WINDOWS" /D "NDEBUG" },
                  LDFLAGS => q{ kernel32.lib /nologo /subsystem:windows /dll /machine:I386 },
                  LIBS => '',
                  EXTRA_INCLUDES => '',
                  APREQ_SOURCE_DIR => $src_dir,
                  APREQ_SO_EXT => $Config{dlext},
                  APREQ_LIB_TARGET => '',
               );

my $apreq_usage = << 'EOF';
Usage: apreq2-config [OPTION]

Known values for OPTION are:
  --prefix[=DIR]    change prefix to DIR
  --bindir          print location where binaries are installed
  --includedir      print location where headers are installed
  --libdir          print location where libraries are installed
  --cc              print C compiler name
  --cpp             print C preprocessor name and any required options
  --ld              print C linker name
  --cflags          print C compiler flags
  --cppflags        print cpp flags
  --includes        print include information
  --ldflags         print linker flags
  --libs            print additional libraries to link against
  --srcdir          print APR-util source directory
  --installbuilddir print APR-util build helper directory
  --link-ld         print link switch(es) for linking to APREQ
  --apreq-so-ext    print the extensions of shared objects on this platform
  --apreq-lib-file  print the name of the apreq lib file
  --version         print the APR-util version as a dotted triple
  --help            print this help

When linking, an application should do something like:
  APREQ_LIBS="\`apreq2-config --link-ld --libs\`"

An application should use the results of --cflags, --cppflags, --includes,
and --ldflags in their build process.
EOF

my $full = catfile $ap_dir{bin}, $file;
open(my $fh, '>', $full) or die "Cannot open $full: $!";
print $fh <<"END";
#!$^X
use strict;
use warnings;
use Getopt::Long;
use File::Spec::Functions qw(catfile catdir);

$license
sub usage {
    print << 'EOU';
$apreq_usage
EOU
    exit(1);
}

END

foreach my $var (keys %apreq_args) {
    print $fh qq{my \${$var} = q[$apreq_args{$var}];\n};
}
print $fh $_ while <DATA>;
close $fh;

my @args = ('pl2bat', $full);
system(@args) == 0 or die "system @args failed: $?";
print qq{apreq2-config.bat has been created under $prefix/bin.\n\n};

__DATA__

my %opts = ();
GetOptions(\%opts,
           'prefix:s',
           'bindir',
           'includedir',
           'libdir',
           'cc',
           'cpp',
           'ld',
           'cflags',
           'cppflags',
           'includes',
           'ldflags',
           'libs',
           'srcdir',
           'installbuilddir',
           'link-ld',
           'apreq-so-ext',
           'apreq-lib-file',
           'version',
           'help'
          ) or usage();

usage() if ($opts{help} or not %opts);

if (exists $opts{prefix} and $opts{prefix} eq "") {
    print qq{$prefix\n};
    exit(0);
}
my $user_prefix = defined $opts{prefix} ? $opts{prefix} : '';
my %user_dir;
if ($user_prefix) {
    foreach (qw(lib bin include build)) {
        $user_dir{$_} = catdir $user_prefix, $_;
    }
}
my $flags = '';

SWITCH : {
    local $\ = "\n";
    $opts{bindir} and do {
        print $user_prefix ? $user_dir{bin} : $bindir;
        last SWITCH;
    };
    $opts{includedir} and do {
        print $user_prefix ? $user_dir{include} : $includedir;
        last SWITCH;
    };
    $opts{libdir} and do {
        print $user_prefix ? $user_dir{lib} : $libdir;
        last SWITCH;
    };
    $opts{installbuilddir} and do {
        print $user_prefix ? $user_dir{build} : $installbuilddir;
        last SWITCH;
    };
    $opts{srcdir} and do {
        print $APREQ_SOURCE_DIR;
        last SWITCH;
    };
    $opts{cc} and do {
        print $CC;
        last SWITCH;
    };
    $opts{cpp} and do {
        print $CPP;
        last SWITCH;
    };
    $opts{ld} and do {
        print $LD;
        last SWITCH;
    };
    $opts{cflags} and $flags .= " $CFLAGS ";
    $opts{cppflags} and $flags .= " $CPPFLAGS ";
    $opts{includes} and do {
        my $inc = $user_prefix ? $user_dir{include} : $includedir;
        $flags .= qq{ /I"$inc" $EXTRA_INCLUDES };
    };
    $opts{ldflags} and $flags .= " $LDFLAGS ";
    $opts{libs} and $flags .= " $LIBS ";
    $opts{'link-ld'} and do {
        my $libpath = $user_prefix ? $user_dir{lib} : $libdir;
        $flags .= qq{ /libpath:"$libpath" $APREQ_LIBNAME };
    };
    $opts{'apreq-so-ext'} and do {
        print $APREQ_SO_EXT;
        last SWITCH;
    };
    $opts{'apreq-lib-file'} and do {
        my $full_apreqlib = $user_prefix ? 
            (catfile $user_dir{lib}, $APREQ_LIBNAME) :
                (catfile $libdir, $APREQ_LIBNAME);
        print $full_apreqlib;
        last SWITCH;
    };
    $opts{version} and do {
        print $APREQ_DOTTED_VERSION;
        last SWITCH;
    };
    print $flags if $flags;
}
exit(0);
