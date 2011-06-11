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

#include "graphics.h"
#include "outverbose.h"
#include "windowgen.h"

Graphics::~Graphics()
{
}

void Graphics::initShared(Outverbose &o)
{
  out = &o;
}

Color Graphics::makeColor(float red, float green, float blue, float alpha)
{
  Color color = { red, green, blue, alpha };
  return color;
}

GraphicsInfo Graphics::makeInfo(float x, float y, float z, float width, float height, float depth, float angleX, float angleY, float angleZ, float pivotX, float pivotY, float pivotZ, float scaleX, float scaleY, float scaleZ, Color color, int texture, std::string text, bool visible)
{
  GraphicsInfo info = { x, y, z, width, height, depth, angleX, angleY, angleZ, pivotX, pivotY, pivotZ, scaleX, scaleY, scaleZ, color, texture, text, visible };
  return info;
}

GraphicsInfo Graphics::defaultInfo()
{
  GraphicsInfo info = { 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, makeColor(1, 1, 1, 1), 0, "", true };
  return info;
}

WindowInfo Graphics::makeWindowInfo(int x, int y, int width, int height, bool doubleBuffer, bool depthBuffer, int refreshRate, int colorDepth, bool fullscreen, std::string title)
{
  return window->makeWindowInfo(x, y, width, height, doubleBuffer, depthBuffer, refreshRate, colorDepth, fullscreen, title);
}

