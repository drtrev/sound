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
 */

#ifndef WINDOWGEN
#define WINDOWGEN

#include <iostream>

/**
 * Store typical window information.
 *
 * \param x the x coordinate.
 * \param y the y coordinate.
 * \param width the window width.
 * \param height the window height.
 * \param doubleBuffer enable/disable double buffer.
 * \param depthBuffer enable/disable depth buffer.
 * \param refreshRate set refresh rate.
 * \param colorDepth set color depth.
 * \param fullscreen set fullscreen true/false.
 * \param title set window title.
 */
struct WindowInfo {
  int x;
  int y;
  int width;
  int height;
  bool doubleBuffer;
  bool depthBuffer;
  int refreshRate;
  int colorDepth;
  bool fullscreen;
  std::string title;
};

class Outverbose;

class WindowGen {
  protected:
    int wid;
    Outverbose* out;
    WindowInfo winfo;

  public:
    WindowGen();
    virtual ~WindowGen();

    WindowInfo makeWindowInfo(int, int, int, int, bool, bool, int, int, bool, std::string);

    int getWid();
    int getWidth();
    int getHeight();
    void resize(int, int);

    void initShared(Outverbose&, WindowInfo);
    virtual bool init(Outverbose&, WindowInfo) = 0;
    virtual void refresh() = 0;
    virtual void destroy() = 0;
};
#endif
