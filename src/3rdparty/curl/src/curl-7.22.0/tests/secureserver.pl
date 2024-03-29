#!/usr/bin/env perl
#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) 1998 - 2010, Daniel Stenberg, <daniel@haxx.se>, et al.
#
# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution. The terms
# are also available at http://curl.haxx.se/docs/copyright.html.
#
# You may opt to use, copy, modify, merge, publish, distribute and/or sell
# copies of the Software, and permit persons to whom the Software is
# furnished to do so, under the terms of the COPYING file.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
#***************************************************************************

# This is the HTTPS, FTPS, POP3S, IMAPS, SMTPS, server used for curl test
# harness. Actually just a layer that runs stunnel properly using the
# non-secure test harness servers.

BEGIN {
    @INC=(@INC, $ENV{'srcdir'}, '.');
}

use strict;
use warnings;
use Cwd;

use serverhelp qw(
    server_pidfilename
    server_logfilename
    );

my $stunnel = "stunnel";

my $verbose=0; # set to 1 for debugging

my $accept_port = 8991; # just our default, weird enough
my $target_port = 8999; # default test http-server port

my $stuncert;

my $ver_major;
my $ver_minor;
my $stunnel_version;
my $socketopt;
my $cmd;

my $pidfile;          # stunnel pid file
my $logfile;          # stunnel log file
my $loglevel = 5;     # stunnel log level
my $ipvnum = 4;       # default IP version of stunneled server
my $idnum = 1;        # dafault stunneled server instance number
my $proto = 'https';  # default secure server protocol
my $conffile;         # stunnel configuration file
my $certfile;         # certificate chain PEM file

#***************************************************************************
# stunnel requires full path specification for several files.
#
my $path   = getcwd();
my $srcdir = $path;
my $logdir = $path .'/log';

#***************************************************************************
# Signal handler to remove our stunnel 4.00 and newer configuration file.
#
sub exit_signal_handler {
    my $signame = shift;
    local $!; # preserve errno
    local $?; # preserve exit status
    unlink($conffile) if($conffile && (-f $conffile));
    exit;
}

#***************************************************************************
# Process command line options
#
while(@ARGV) {
    if($ARGV[0] eq '--verbose') {
        $verbose = 1;
    }
    elsif($ARGV[0] eq '--proto') {
        if($ARGV[1]) {
            $proto = $ARGV[1];
            shift @ARGV;
        }
    }
    elsif($ARGV[0] eq '--accept') {
        if($ARGV[1]) {
            if($ARGV[1] =~ /^(\d+)$/) {
                $accept_port = $1;
                shift @ARGV;
            }
        }
    }
    elsif($ARGV[0] eq '--connect') {
        if($ARGV[1]) {
            if($ARGV[1] =~ /^(\d+)$/) {
                $target_port = $1;
                shift @ARGV;
            }
        }
    }
    elsif($ARGV[0] eq '--stunnel') {
        if($ARGV[1]) {
            $stunnel = $ARGV[1];
            shift @ARGV;
        }
    }
    elsif($ARGV[0] eq '--srcdir') {
        if($ARGV[1]) {
            $srcdir = $ARGV[1];
            shift @ARGV;
        }
    }
    elsif($ARGV[0] eq '--certfile') {
        if($ARGV[1]) {
            $stuncert = $ARGV[1];
            shift @ARGV;
        }
    }
    elsif($ARGV[0] eq '--id') {
        if($ARGV[1]) {
            if($ARGV[1] =~ /^(\d+)$/) {
                $idnum = $1 if($1 > 0);
                shift @ARGV;
            }
        }
    }
    elsif($ARGV[0] eq '--ipv4') {
        $ipvnum = 4;
    }
    elsif($ARGV[0] eq '--ipv6') {
        $ipvnum = 6;
    }
    elsif($ARGV[0] eq '--pidfile') {
        if($ARGV[1]) {
            $pidfile = "$path/". $ARGV[1];
            shift @ARGV;
        }
    }
    elsif($ARGV[0] eq '--logfile') {
        if($ARGV[1]) {
            $logfile = "$path/". $ARGV[1];
            shift @ARGV;
        }
    }
    else {
        print STDERR "\nWarning: secureserver.pl unknown parameter: $ARGV[0]\n";
    }
    shift @ARGV;
}

#***************************************************************************
# Initialize command line option dependant variables
#
if(!$pidfile) {
    $pidfile = "$path/". server_pidfilename($proto, $ipvnum, $idnum);
}
if(!$logfile) {
    $logfile = server_logfilename($logdir, $proto, $ipvnum, $idnum);
}

$conffile = "$path/stunnel.conf";

$certfile = "$srcdir/". ($stuncert?"certs/$stuncert":"stunnel.pem");

my $ssltext = uc($proto) ." SSL/TLS:";

#***************************************************************************
# Find out version info for the given stunnel binary
#
foreach my $veropt (('-version', '-V')) {
    foreach my $verstr (qx($stunnel $veropt 2>&1)) {
        if($verstr =~ /^stunnel (\d+)\.(\d+) on /) {
            $ver_major = $1;
            $ver_minor = $2;
            last;
        }
    }
    last if($ver_major);
}
if((!$ver_major) || (!$ver_minor)) {
    if(-x "$stunnel" && ! -d "$stunnel") {
        print "$ssltext Unknown stunnel version\n";
    }
    else {
        print "$ssltext No stunnel\n";
    }
    exit 1;
}
$stunnel_version = (100*$ver_major) + $ver_minor;

#***************************************************************************
# Verify minimmum stunnel required version
#
if($stunnel_version < 310) {
    print "$ssltext Unsupported stunnel version $ver_major.$ver_minor\n";
    exit 1;
}

#***************************************************************************
# Build command to execute for stunnel 3.X versions
#
if($stunnel_version < 400) {
    if($stunnel_version >= 319) {
        $socketopt = "-O a:SO_REUSEADDR=1";
    }
    $cmd  = "$stunnel -p $certfile -P $pidfile ";
    $cmd .= "-d $accept_port -r $target_port -f -D $loglevel ";
    $cmd .= ($socketopt) ? "$socketopt " : "";
    $cmd .= ">$logfile 2>&1";
    if($verbose) {
        print uc($proto) ." server (stunnel $ver_major.$ver_minor)\n";
        print "cmd: $cmd\n";
        print "pem cert file: $certfile\n";
        print "pid file: $pidfile\n";
        print "log file: $logfile\n";
        print "log level: $loglevel\n";
        print "listen on port: $accept_port\n";
        print "connect to port: $target_port\n";
    }
}

#***************************************************************************
# Build command to execute for stunnel 4.00 and newer
#
if($stunnel_version >= 400) {
    $socketopt = "a:SO_REUSEADDR=1";
    $cmd  = "$stunnel $conffile ";
    $cmd .= ">$logfile 2>&1";
    # setup signal handler
    $SIG{INT} = \&exit_signal_handler;
    $SIG{TERM} = \&exit_signal_handler;
    # stunnel configuration file
    if(open(STUNCONF, ">$conffile")) {
	print STUNCONF "
	CApath = $path
	cert = $certfile
	pid = $pidfile
	debug = $loglevel
	output = $logfile
	socket = $socketopt
	foreground = yes
	
	[curltest]
	accept = $accept_port
	connect = $target_port
	";
        if(!close(STUNCONF)) {
            print "$ssltext Error closing file $conffile\n";
            exit 1;
        }
    }
    else {
        print "$ssltext Error writing file $conffile\n";
        exit 1;
    }
    if($verbose) {
        print uc($proto) ." server (stunnel $ver_major.$ver_minor)\n";
        print "cmd: $cmd\n";
        print "CApath = $path\n";
        print "cert = $certfile\n";
        print "pid = $pidfile\n";
        print "debug = $loglevel\n";
        print "output = $logfile\n";
        print "socket = $socketopt\n";
        print "foreground = yes\n";
        print "\n";
        print "[curltest]\n";
        print "accept = $accept_port\n";
        print "connect = $target_port\n";
    }
}

#***************************************************************************
# Set file permissions on certificate pem file.
#
chmod(0600, $certfile) if(-f $certfile);

#***************************************************************************
# Run stunnel.
#
my $rc = system($cmd);

$rc >>= 8;

unlink($conffile) if($conffile && -f $conffile);

exit $rc;
