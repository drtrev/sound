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

#ifndef INPUT_SDL
#define INPUT_SDL

#include "input.h"
#ifdef _MSC_VER
#include <sdl.h>
#else
#include <SDL/SDL.h>
#endif

/**
 * Handle keyboard input using SDL.
 */
class InputSDL : public Input {
  private:
    int map(SDLKey, bool keydown); /**< Map SDL key to our key (defined in input.h) */
    int mapMouse(Uint8, bool keydown); /**< Map SDL key to our key (defined in input.h) */

  public:
    void grab(); /**< Grab keyboard (just set grabbed to true in this case). */

    int check(int keys, int &mousexrel, int &mouseyrel, int windowWidth, int windowHeight); /**< Check for keypress/release events. */

    void release(); /**< Release keyboard. */

};

#endif
