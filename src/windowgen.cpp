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

#include "windowgen.h"

#include <iostream>

WindowGen::WindowGen()
{
  wid = -1;
}

WindowGen::~WindowGen()
{
}

WindowInfo WindowGen::makeWindowInfo(int x, int y, int width, int height, bool doubleBuffer, bool depthBuffer, int refreshRate, int colorDepth, bool fullscreen, std::string title)
{
  WindowInfo info = { x, y, width, height, doubleBuffer, depthBuffer, refreshRate, colorDepth, fullscreen, title };
  return info;
}

int WindowGen::getWid()
{
  return wid;
}

int WindowGen::getWidth()
{
  return winfo.width;
}

int WindowGen::getHeight()
{
  return winfo.height;
}

void WindowGen::resize(int w, int h)
{
  winfo.width = w;
  winfo.height = h;
}

void WindowGen::initShared(Outverbose& o, WindowInfo w)
{
  out = &o;
}

