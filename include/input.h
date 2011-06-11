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

#ifndef INPUT
#define INPUT

// Define our keyboard events.
// App just needs to know what we're trying to do, not what key we're
// pressing. Actual keys are referred to using SDL constants.

#define KEYS_UP        0x0001
#define KEYS_LEFT      0x0002
#define KEYS_DOWN      0x0004
#define KEYS_RIGHT     0x0008
#define KEYS_FIRE      0x0010
#define KEYS_QUIT      0x0020
#define KEYS_TALK      0x0040
#define KEYS_NEXT      0x0080
#define KEYS_BACK      0x0100
#define KEYS_FORWARD   0x0200
#define KEYS_BACKWARD  0x0400
#define KEYS_JUMP      0x0800
#define KEYS_CONSTRUCT 0x1000

/**
 * The input class.
 *
 * Define key operations, and optionally grab keyboard. Designed to be extended
 * (e.g. by using Xlib or SDL).
 */
class Input {
  protected:
    bool grabbed; /**< Store whether keyboard is grabbed. */
    bool mouselocked; /**< Is the mouse locked/warped to the centre? */

  public:
    Input(); /**< Constructor. */

    bool getGrabbed(); /**< Check if keyboard is grabbed. \return true if grabbed, false otherwise. */

    void mouseLock(bool);

    //virtual int check(int) = 0;
};

#endif
