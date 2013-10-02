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

# function pointer declaration
sub make_pfn_decl($%)
{
    our $export;
    return $export . " PFN" . (uc $_[0]) . "PROC " . prefixname($_[0]) . ";";
}

my @extlist = ();
my %extensions = ();

our $export = shift;

if (@ARGV)
{
    @extlist = @ARGV;

	foreach my $ext (sort @extlist)
	{
		my ($extname, $exturl, $extstring, $types, $tokens, $functions, $exacts) = parse_ext($ext);
		output_decls($functions, \&make_pfn_decl);
	}
}
