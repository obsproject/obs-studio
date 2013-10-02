#!/usr/bin/perl
##
## Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
## Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
##
## This program is distributed under the terms and conditions of the GNU
## General Public License Version 2 as published by the Free Software
## Foundation or, at your option, any later version.

use strict;
use warnings;

do 'bin/make.pl';

#---------------------------------------------------------------------------------------

# function pointer definition
sub make_pfn_def($%)
{
    return "PFN" . (uc $_[0]) . "PROC " . prefixname($_[0]) . " = NULL;";
}

# function pointer definition
sub make_init_call($%)
{
    my $name = prefixname($_[0]);
    return "  r = r || (" . $name . " = (PFN" . (uc $_[0]) . "PROC)glewGetProcAddress((const GLubyte*)\"" . $name . "\")) == NULL;";
}

#---------------------------------------------------------------------------------------

my @extlist = ();
my %extensions = ();

if (@ARGV)
{
    @extlist = @ARGV;

	foreach my $ext (sort @extlist)
	{
		my ($extname, $exturl, $extstring, $types, $tokens, $functions, $exacts) = parse_ext($ext);

		print "#ifdef $extname\n";
		print "  _glewInfo_$extname();\n";
		print "#endif /* $extname */\n";
	}
}
