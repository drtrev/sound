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

#ifndef _MSC_VER
#ifndef INPUT_X11
#define INPUT_X11

// Inspiration from: http://www.xmission.com/~georgeps/documentation/tutorials/Xlib_Beginner.html

#include "input.h"
#include <X11/Xlib.h>

/**
 * Handle keyboard input using Xlib.
 */
class InputX11 : public Input {
  private:
    Display *dis;  /**< The display we're associated with. */
    Window win;    /**< The window we're associated with. */
    XEvent report; /**< The event report. */

    int map(int);  /**< Map from X11 keyboard symbol to our own definitions, defined in input.h */

  public:
    void grab();    /**< Grab control of keyboard. */

    int check(int); /**< Check for keypress/release events. */

    void release(); /**< Release control of keyboard. */
};

#endif
#endif

