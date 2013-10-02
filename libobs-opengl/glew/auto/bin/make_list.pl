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
sub make_init_call($%)
{
    my $name = prefixname($_[0]);
    return "  r = r || (" . $_[0] . " = (PFN" . (uc $_[0]) . "PROC)glewGetProcAddress(\"" . $name . "\")) == NULL;";
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

		my $extvar = $extname;
		$extvar =~ s/GL(X*)_/GL$1EW_/;

		my $extpre = $extname;
		$extpre =~ s/^(W?)GL(X?).*$/\l$1gl\l$2ew/;

		#my $pextvar = prefix_varname($extvar);

		print "#ifdef $extname\n";

		if (length($extstring))
		{
				print "  CONST_CAST(" . $extvar . ") = _glewSearchExtension(\"$extstring\", extStart, extEnd);\n";
		}

		if (keys %$functions)
		{
			if ($extname =~ /WGL_.*/)
			{
				print "  if (glewExperimental || " . $extvar . "|| crippled) CONST_CAST(" . $extvar . ")= !_glewInit_$extname(GLEW_CONTEXT_ARG_VAR_INIT);\n";
			}
			else
			{
				print "  if (glewExperimental || " . $extvar . ") CONST_CAST(" . $extvar . ") = !_glewInit_$extname(GLEW_CONTEXT_ARG_VAR_INIT);\n";
			}
		}
		print "#endif /* $extname */\n";
	}

}
