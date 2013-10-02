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
sub make_pfn_info($%)
{
    my $name = $_[0];
    return "  glewInfoFunc(\"$_[0]\", $name == NULL);";
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
		$extpre =~ s/(GLX|GLW|GL).*/$1/;
		$extpre = lc $extpre;

		#make_separator($extname);
		print "#ifdef $extname\n\n";
		print "static void _glewInfo_$extname (void)\n{\n";
		if ($extvar =~ /VERSION/)
		{
			print "  glewPrintExt(\"$extname\", $extvar, $extvar, $extvar);\n";
		}
		else
		{
			print "  glewPrintExt(\"$extname\", $extvar, $extpre" .
				"ewIsSupported(\"$extname\"), $extpre" .
				"ewGetExtension(\"$extstring\"));\n";
		}
		output_decls($functions, \&make_pfn_info);
		print "}\n\n";
		print "#endif /* $extname */\n\n";
	}
}
