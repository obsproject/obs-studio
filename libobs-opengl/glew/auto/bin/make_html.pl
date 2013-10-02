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

my @extlist = ();
my %extensions = ();
my $group = "";
my $cur_group = "";

if (@ARGV)
{
    @extlist = @ARGV;
	my $n = 1;
	print "<table border=\"0\" width=\"100%\" cellpadding=\"1\" cellspacing=\"0\" align=\"center\">\n";
	foreach my $ext (sort @extlist)
	{
		my ($extname, $exturl, $extstring, $types, $tokens, $functions, $exacts) = parse_ext($ext);
		$cur_group = $extname;
		$cur_group =~ s/^(?:W?)GL(?:X?)_([A-Z0-9]+?)_.*$/$1/;
		$extname =~ s/^(?:W?)GL(?:X?)_(.*)$/$1/;
		if ($cur_group ne $group)
		{
			if ($group ne "")
			{
				print "<tr><td><br></td><td></td><td></td></tr>\n";
			}
			$group = $cur_group;
		}

		{
			if ($exturl)
			{
				print "<tr><td class=\"num\">$n</td><td>&nbsp;</td><td><a href=\"$exturl\">$extname</a></td></tr>\n";
			}
			else
			{
				print "<tr><td class=\"num\">$n</td><td>&nbsp;</td><td>$extname</td></tr>\n";
			}
			$n++;
		}
	}
	print "</table>\n"
}
