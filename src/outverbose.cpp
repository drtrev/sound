/* This file is part of Map - A tool for viewing 2D map data using Motlab (a
 * heterogeneous collaborative visualization library).
 *
 * Copyright (C) 2009 Trevor Dodds <trev@comp.leeds.ac.uk>
 *
 * Map is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 * 
 * Map is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * Map.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 */

#include "outverbose.h"
#include <iostream>
#include <string>

using std::string;
using std::cerr;
using std::endl;

Outverbose::Outverbose()
{
  verbose = VERBOSE_NORMAL; // the user setting
  outVerbosity = VERBOSE_NORMAL; // the programmers setting
  //err = STREAM_STDOUT;
}

Outverbose::~Outverbose()
{
  cerr.flush();
}

bool Outverbose::checkVerbosity(verboseEnum v)
{
  if (v > 1 - verbose) return true;
  return false;
}

void Outverbose::setVerbosity(verboseEnum v)
{
  verbose = v;
}

void Outverbose::outputVerbosity(verboseEnum v)
  // output verbose level
{
  switch (verbose) {
    case VERBOSE_QUIET:
      addln("Quiet", v);
      break;
    case VERBOSE_NORMAL:
      addln("Normal", v);
      break;
    case VERBOSE_LOUD:
      addln("Loud", v);
      break;
    default:
      addln("Invalid value", v);
  }
}

void Outverbose::add(const char* msg, verboseEnum v)
{
  if (checkVerbosity(v)) Out::add(msg);
}

void Outverbose::add(string msg, verboseEnum v)
{
  if (checkVerbosity(v)) add(msg);
}

void Outverbose::add(char c, verboseEnum v)
{
  if (checkVerbosity(v)) add(c);
}

void Outverbose::add(int i, verboseEnum v)
{
  if (checkVerbosity(v)) add(i);
}

void Outverbose::addln(int i, verboseEnum v)
{
  if (checkVerbosity(v)) addln(i);
}

void Outverbose::addln(const char* msg, verboseEnum v)
{
  if (checkVerbosity(v)) addln(msg);
}

void Outverbose::addln(string msg, verboseEnum v)
{
  if (checkVerbosity(v)) addln(msg);
}


// add to stderr also

void Outverbose::adderr(const char* msg, verboseEnum v)
{
  if (checkVerbosity(v)) {
    add(msg);
    cerr << msg;
  }
}

void Outverbose::adderr(string msg, verboseEnum v)
{
  if (checkVerbosity(v)) {
    add(msg);
    cerr << msg;
  }
}

void Outverbose::adderr(char c, verboseEnum v)
{
  if (checkVerbosity(v)) {
    add(c);
    cerr << c;
  }
}

void Outverbose::adderr(int i, verboseEnum v)
{
  if (checkVerbosity(v)) {
    add(i);
    cerr << i;
  }
}

void Outverbose::adderrln(int i, verboseEnum v)
{
  if (checkVerbosity(v)) {
    addln(i);
    cerr << i << endl;
  }
}

void Outverbose::adderrln(const char* msg, verboseEnum v)
{
  if (checkVerbosity(v)) {
    addln(msg);
    cerr << msg << endl;
  }
}

void Outverbose::adderrln(string msg, verboseEnum v)
{
  if (checkVerbosity(v)) {
    addln(msg);
    cerr << msg << endl;
  }
}

// overload stream-insertion operator
// note that endl is actually a function in ostream, which
// prints a newline character and flushes the output stream

Outverbose& Outverbose::operator<<(const verboseEnum v)
{
  outVerbosity = v;
  return *this;
}

Outverbose& Outverbose::operator<<(const char* str)
{
  add(str, outVerbosity);
  return *this;
}

Outverbose& Outverbose::operator<<(const string &str)
{
  add(str, outVerbosity);
  return *this;
}

Outverbose& Outverbose::operator<<(const char c)
{
  add(c, outVerbosity);
  return *this;
}

Outverbose& Outverbose::operator<<(const int i)
{
  add(i, outVerbosity);
  return *this;
}

