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
my $match = '';

if (@ARGV < 1 || $ARGV[0] eq "-h") {
  print "Usage: ./addMatch.pl [-p path] <expression>\n";
  print "Default path is ~/ogg\n";
  exit 0;
}

if ($ARGV[0] eq "-p" && @ARGV > 1) {
  $path = shift(@ARGV); # -p
  $path = shift(@ARGV);
}

if (@ARGV < 1 || $ARGV[0] eq "-h") {
  print "Usage: ./addMatch.pl [-p path] <expression>\n";
  print "Default path is ~/ogg\n";
  exit 0;
}

my $match = shift(@ARGV);

# stick everything in a file
print "find $path -name *.ogg > playlistTemp\n";
system("find $path -name *.ogg > playlistTemp");

print "perl -pi -e 'print;s#.*/(.*)\..*#\$1#' playlistTemp\n";
system('perl -pi -e \'print;s#.*/(.*)\..*#$1#\' playlistTemp');


# copy the ones that match
my @playlist;

open (PLAYLIST, "<playlistTemp") || die "Can't open temporary playlist file! Error: $!\n";

my $readingFile = 1; # first line is a file
my $addNext = 0;
my $added = 0; # number of files added

while (<PLAYLIST>) {
  if ($addNext || $_ =~ $match) {
    push(@playlist, $_);
    if ($readingFile) {
      $addNext = 1;
    }
    $readingFile = !$readingFile;
  }

# UP TO HERE!!!!!!
  if (!$addNext && (!$readingFile || $_ !~ $match)) {
    push(@playlist, $_);
  }else{
    if ($readingFile) {
      $skipped++;
      $skipNext = 1; # skip title
    }else{
      $skipNext = 0;
      print "Removing $_";
    }
  }
  $readingFile = !$readingFile;
}

close(PLAYLIST);

open (PLAYLIST, ">playlist");
foreach (@playlist) {
  print PLAYLIST $_;
}
close(PLAYLIST);

print "Removed $skipped files.\n";
