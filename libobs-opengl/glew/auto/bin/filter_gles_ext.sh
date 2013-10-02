#!/bin/bash
##
## Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
## Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
##
## This program is distributed under the terms and conditions of the GNU
## General Public License Version 2 as published by the Free Software
## Foundation or, at your option, any later version.
##
## Parameters:
##
##       $1: Extensions directory
##       $2: Registry directory
##       $3: The black list

set -e

# clean up
    rm -f $1/*.bak
