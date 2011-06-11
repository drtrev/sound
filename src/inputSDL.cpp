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

#include "inputSDL.h"
#include <iostream>

void InputSDL::grab()
{
  grabbed = true;
}

int InputSDL::map(SDLKey sym, bool keydown)
{
  int key = 0;

  switch (sym) {
    case SDLK_UP:
      key = KEYS_FORWARD;
      break;
    case SDLK_LEFT:
      key = KEYS_LEFT;
      break;
    case SDLK_RIGHT:
      key = KEYS_RIGHT;
      break;
    case SDLK_DOWN:
      key = KEYS_BACKWARD;
      break;
    case SDLK_LCTRL:
      key = KEYS_FIRE;
      break;
    case SDLK_SPACE:
      key = KEYS_TALK;
      break;
    case SDLK_b:
      key = KEYS_BACK;
      break;
    case SDLK_PAGEDOWN:
      key = KEYS_DOWN;
      break;
    case SDLK_l:
      // local input
      if (keydown) mouselocked = !mouselocked;
      break;
    case SDLK_n:
      key = KEYS_NEXT;
      break;
    case SDLK_PAGEUP:
      key = KEYS_UP;
      break;
    case SDLK_ESCAPE:
      key = KEYS_QUIT;
      break;
    default:
      break;
  }

  return key;
}

int InputSDL::mapMouse(Uint8 button, bool keydown)
{
  int key = 0;

  switch (button)
  {
    case SDL_BUTTON_LEFT:
      key = KEYS_FIRE;
      break;
    case SDL_BUTTON_MIDDLE:
      key = KEYS_JUMP;
      //std::cout << "Jump" << std::endl;
      break;
    case SDL_BUTTON_RIGHT:
      key = KEYS_CONSTRUCT;
      break;
    default:
      break;
  }

  return key;
}

int InputSDL::check(int keys, int &mousexrel, int &mouseyrel, int windowWidth, int windowHeight)
{
  int key = 0;

  // see: http://www.libsdl.org/intro.en/usingevents.html
  //SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

  SDL_Event event;

  // TODO this loop will get press and release - could process each event separately

  mousexrel = 0, mouseyrel = 0;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_KEYDOWN:
        key = map(event.key.keysym.sym, true);
        keys |= key;
        break;
      case SDL_KEYUP:
        key = map(event.key.keysym.sym, false);
        keys &= ~key;
        break;
      case SDL_MOUSEBUTTONDOWN:
        key = mapMouse(event.button.button, true);
        keys |= key;
        break;
      case SDL_MOUSEBUTTONUP:
        key = mapMouse(event.button.button, false);
        keys &= ~key;
        break;
      case SDL_VIDEORESIZE:
        std::cerr << "Resize event received" << std::endl;
        // TODO call window.resize(event.w, event.h);
        break;
      case SDL_MOUSEMOTION:
        if (mouselocked) {
          mousexrel += event.motion.xrel; // accumulate
          mouseyrel += event.motion.yrel;
        }
        break;
    }
  }

  if (mouselocked && (mousexrel != 0 || mouseyrel != 0)) {
    // warp pointer back, it's necessary to ignore this motion!
    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
    SDL_WarpMouse(windowWidth/2, windowHeight/2);
    SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
  }

  return keys;
}

void InputSDL::release()
{
  grabbed = false;
}
