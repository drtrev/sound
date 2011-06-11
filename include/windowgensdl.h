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

#ifndef WINDOWGEN_SDL
#define WINDOWGEN_SDL

#include "windowgen.h"

/**
 * Generate a window using SDL.
 */

class WindowSDL : public WindowGen {
  public:
    /**
     * Initialisation.
     *
     * Open a window based on the window information provided.
     *
     * \param o the instance of the output object.
     * \param w the window information.
     * \return true on success, false on failure.
     */
    bool init(Outverbose &o, WindowInfo w);
    void refresh(); /**< It's double buffered, so swap buffers. */
    void destroy(); /**< Destroy the window. */
};

#endif
