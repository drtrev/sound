#!/usr/bin/perl
# sound - 3D sound project
#
# Copyright (C) 2011 Trevor Dodds <@gmail.com trev.dodds>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# Generate 'playlist' file
# Usage: ./genplaylist.pl [path]
# Default path is ~/ogg

use warnings;
use strict;

my $path = '~/ogg';

if (@ARGV > 0) {
  if ($ARGV[0] eq "-h") {
    print "Usage: ./genplaylist.pl [path]\n";
    print "Default path is ~/ogg\n";
    exit 0;
  }
  $path = shift(@ARGV);
}

print "find $path -name \\*.ogg > playlist\n";
system("find $path -name \\*.ogg > playlist");

print "perl -pi -e 'print;s#.*/(.*)\..*#\$1#' playlist\n";
system('perl -pi -e \'print;s#.*/(.*)\..*#$1#\' playlist');
