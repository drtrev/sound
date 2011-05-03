/* sound - 3D sound project
 *
 * Copyright (C) 2011 Trevor Dodds <@gmail.com trev.dodds>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _BUFFER_
#define _BUFFER_

#include <iostream>

//WORKS with FLAG_AUDIO_SIZE 500 #define BUFFER_SIZE 2000
//WORKS with FLAG_AUDIO_SIZE 2000 #define BUFFER_SIZE 5000
#define BUFFER_SIZE 15000 // this would prob work at 4000 and a bit - if not error messages will confirm
// this must store at least two data units in case one is incomplete and another needs to take it's place
// (i.e. first one will be skipped when next ASCII_START_DATA is found, but need room for next one otherwise
// end up back at square one with an incomplete data unit!)

using namespace std;

class Buffer {
  private:
    char buffer[BUFFER_SIZE];
    int writeCursor, readCursor, dataOnBuffer;

  public:
    Buffer();                    // constructor
    int write(const char*, const int);      // returns the number of bytes successfully written
    int read(char[], int);       // reads into first parameter (removing from buffer). returns no of bytes successfully read
    bool getChar(char&, int);    // copies a char into the first parameter. Returns true on success

    int getDataOnBuffer() const; // returns the amount of data that is on the buffer
    
};

#endif
