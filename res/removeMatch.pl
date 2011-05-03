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

# Remove a particular match in filename (e.g. artist/album/song) from playlist
# Usage: ./removeMatch.pl <expression>

use warnings;
use strict;

if (@ARGV < 1 || $ARGV[0] eq "-h") {
  print "Usage: ./removeMatch.pl <expression>\n";
  exit 0;
}

my $match = shift(@ARGV);
my @playlist;

open (PLAYLIST, "<playlist") || die "Can't open playlist file! Error: $!\n";

my $readingFile = 1; # first line is a file
my $skipNext = 0;
my $skipped = 0; # number of files skipped

while (<PLAYLIST>) {
  if (!$skipNext && (!$readingFile || $_ !~ $match)) {
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
