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

#ifndef _MSC_VER
#include "inputX11.h"
//#include <cstdlib>
#include <iostream>
#include <X11/keysym.h>

using std::cout;
using std::cerr;
using std::endl;

void InputX11::grab()
{
  dis = XOpenDisplay(NULL);
  if (dis == NULL) {
    cerr << "Could not open display. Exiting..." << endl;
  }
  win = DefaultRootWindow(dis);

  // don't use Sync, it takes over!
  XGrabKeyboard(dis, win, False, GrabModeAsync, GrabModeAsync, CurrentTime);
  XAutoRepeatOff(dis);

  grabbed = true;
}

int InputX11::map(int key)
// convert x key number to our own number
{
  switch (key) {
    case XK_Up:
      key = KEYS_FORWARD;
      break;
    case XK_Left:
      key = KEYS_LEFT;
      break;
    case XK_Down:
      key = KEYS_BACKWARD;
      break;
    case XK_Right:
      key = KEYS_RIGHT;
      break;
    case XK_Control_L:
      key = KEYS_FIRE;
      break;
    case XK_space:
      key = KEYS_TALK;
      break;
    case XK_b:
      key = KEYS_BACK;
      break;
    case XK_l:
      key = KEYS_DOWN;
      break;
    case XK_n:
      key = KEYS_NEXT;
      break;
    case XK_p:
      key = KEYS_UP;
      break;
    case XK_q:
      key = KEYS_QUIT;
      break;
    default:
      key = 0; // no key
      break;
  }

  return key;
}

int InputX11::check(int keys)
{
  // if there's any events
  while (XEventsQueued(dis, QueuedAfterFlush) > 0) {

    XNextEvent(dis, &report);
    KeySym key;

    switch  (report.type) {
      case KeyPress:
        key = XLookupKeysym(&report.xkey, 0);

        // Close the program if q is pressed.
        /*if (key == XK_q) {
          XUngrabKeyboard(dis, CurrentTime);
          exit(0);
        }*/

        key = map(key);
        keys |= key; // add key
        break;

      case KeyRelease:
        key = XLookupKeysym(&report.xkey, 0);

        key = map(key);
        // remember '!' treats the value as boolean, use bitwise not '~'
        keys &= ~key; // remove key
        break;
    }
  }

  return keys;
}

void InputX11::release()
{
  XAutoRepeatOn(dis);
  XUngrabKeyboard(dis, CurrentTime);
  XCloseDisplay(dis);
  cout << "Released keyboard" << endl;

  grabbed = false;
}
#endif
