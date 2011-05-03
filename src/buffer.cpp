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

#include "buffer.h"

Buffer::Buffer()
{
  readCursor = 0, writeCursor = 0, dataOnBuffer = 0;

  for (int i = 0; i < BUFFER_SIZE; i++) {
    buffer[i] = 0; // NULL
  }
}

// returns the number of bytes successfully written
int Buffer::write(const char* str, const int len)
{
  int numBytes = 0;
  
  for (int i = 0; i < len; i++) {
    // have we reached capacity?
    if (dataOnBuffer == BUFFER_SIZE) {
      cerr << "Buffer::write(): Attempt to write past buffer capacity." << endl;
      break;
    }
    buffer[writeCursor++] = str[i];
    if (writeCursor == BUFFER_SIZE) writeCursor = 0;
    dataOnBuffer++; // we've added a bit
    numBytes++;
  }
  
  return numBytes;
}

// read n characters from buffer into readStr
// returns number of characters successfully read
int Buffer::read(char readStr[], int n)
{
  int numRead = 0;

  for (int i = 0; i < n; i++) {
    // readCursor will be more than writeCursor if writeCursor has looped round
    // so keep track of data left using variable
    if (dataOnBuffer < 1) {
      cerr << "Buffer::read(): no data left to read." << endl;
      break;
    }
  
    readStr[i] = buffer[readCursor++];
    if (readCursor == BUFFER_SIZE) readCursor = 0;
    dataOnBuffer--; // we've read a bit
    numRead++;
  }

  return numRead;
}

// look at a single character.
bool Buffer::getChar(char &c, int offset)
{
  if (offset >= dataOnBuffer) {
    cerr << "Buffer::getChar: attempt to read past end of data" << endl;
    return false;
  }
  
  int position = readCursor+offset;
  if (position >= BUFFER_SIZE) position -= BUFFER_SIZE;
  c = buffer[position];

  return true;
}

// return the amount of data on the buffer
int Buffer::getDataOnBuffer() const
{
  return dataOnBuffer;
}
