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

#include "outstd.h"
#include <iostream>

#ifdef _MSC_VER
#include <stdio.h>
#define snprintf sprintf_s
#endif

using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::string;

Out::Out()
{
  initialised = false;
  openedLog = false;

  cols = 157;
  lines = 55;
}

Out::~Out()
{
  if (openedLog) closeLog();
}

int Out::getWidth()
{
  return cols;
}

int Out::getHeight()
{
  return lines;
}

void Out::init()
{
  outputX = 0;
  outputY = 0;
  inputX = 0;
  inputY = 0; // initialised below, after the initscr has determined COLS and LINES
  cursor = 0;
  inputLine = 0;
  inputChar = 0;
  outputLine = 0;
  outputChar = 0;

  menuPos = lines - 10;
  inputY = menuPos+1;
}

void Out::endWin()
{
  if (initialised) {
    //endwin();
    initialised = false;
  }
}

void Out::addCh(int y, int x, char c)
{
  cout << c;
}

void Out::add(const char* msg)
{
  if (openedLog) logfile << msg;

  cout << msg;
}

void Out::add(string msg)
{
  add(msg.c_str());
}

void Out::add(char c)
{
  char cArray[2];
  cArray[0] = c;
  cArray[1] = 0;
  add(cArray);
}

void Out::add(int n)
{
  char c[20];
#ifdef _MSC_VER
  sprintf(c, "%i", n); // insecure!
#else
  snprintf(c, 20, "%i", n);
#endif
  add(c);
}

void Out::addln(int n)
{
  add(n);
  add("\n");
}

void Out::addln(const char* msg)
{
  add(msg);
  add("\n");
}

void Out::addln(string msg)
{
  add(msg.c_str());
  add("\n");
}

// overload stream-insertion operator
// note that endl is actually a function in ostream, which
// prints a newline character and flushes the output stream

/*Out& Out::operator<<(const char* str)
{
  add(str);
  return *this;
}

Out& Out::operator<<(const string &str)
{
  add(str);
  return *this;
}*/

void Out::refreshScreen()
  // note this is not needed if using getCh
{
  //refresh();
}

int Out::getCh()
{
  // this also performs refresh
  //return getch();
  return getchar();
}

char Out::get()
{
  //return getIn(getch());
  return '\0';
}

void Out::putIn(const char* str)
{
  //for (int i = 0; i < (int) strlen(str); i++) getIn(str[i]);
}

char Out::getIn(int c)
{
  /*// c could be \n if called using putIn
  if (c == 13 || c == '\n') { // enter key (ASCII 13) - the constant in curses.h doesn't work
    inputX = 0;
    inputY++;
    inputLine++;
    if (inputLine < MAX_LINES) {
      // blank the new line
      inputLines[inputLine][0] = 0;
    }
    inputChar = 0;
    if (inputY > LINES - 1) scrollInput();
    move(inputY, inputX);
    c = '\n';
  }else if (c == KEY_BACKSPACE) {
    if (inputX > 0) {
      inputX--;
      mvaddch(inputY, inputX, ' ');
      move(inputY, inputX);
      if (inputChar > 0) {
        inputChar--;
        inputLines[inputLine][inputChar] = 0;
      }else{
        addln("Error: inputChar out of sync with inputX");
        cerr << "Error: inputChar out of sync with inputX" << endl;
      }
    }
    c = 8; // ASCII backspace
  }else{
    // echo input
    if (inputChar < MAX_COLS - 1) {
      // allow room for terminating null

      if (c > 31) {
        // if displayable
        if (inputX >= COLS) {
          // return
          inputX = 0;
          inputY++;
          inputLine++;
          if (inputLine < MAX_LINES) {
            // blank the new line
            inputLines[inputLine][0] = 0;
          }
          inputChar = 0;
          if (inputY > LINES - 1) scrollInput();
          move(inputY, inputX);
        }
        mvaddch(inputY, inputX++, c);
        inputLines[inputLine][inputChar] = c;
        inputChar++;
        inputLines[inputLine][inputChar] = 0; // terminate
      }
    }else{
      addln("Input line too long");
      cerr << "Input line too long" << endl;
    }
  }
  refresh();
*/
  return c;
}

void Out::scrollInput()
{
  /*// shift input lines up one
  for (int i = 0; i < inputLine - 1; i++) {
    for (int j = 0; j < MAX_COLS; j++) inputLines[i][j] = inputLines[i+1][j];
  }
  inputLine--;
  inputY--;

  for (int i = 0; i < inputLine; i++) {
    mvaddstr(menuPos+1+i, 0, inputLines[i]);
    for (int j = strlen(inputLines[i]); j < COLS; j++) {
      // blank the rest
      mvaddch(menuPos+1+i, j, ' ');
    }
  }
  for (int i = 0; i < COLS; i++) mvaddch(menuPos+1+inputLine, i, ' ');

  // blank the old line that we've moved back to
  inputLines[inputLine][0] = 0;

  refresh();*/
}

void Out::scrollOutput()
{
  /*// shift output lines up one
  for (int i = 0; i < outputLine - 1; i++) {
    for (int j = 0; j < MAX_COLS; j++) outputLines[i][j] = outputLines[i+1][j];
  }
  outputLine--;
  outputY--;

  for (int i = 0; i < outputLine; i++) {
    if (strlen(outputLines[i]) > 0) mvaddstr(i, 0, outputLines[i]);
    for (int j = strlen(outputLines[i]); j < COLS; j++) {
      // blank the rest
      mvaddch(i, j, ' ');
    }
  }
  for (int i = 0; i < COLS; i++) mvaddch(outputLine, i, ' ');
  refresh();*/
}

void Out::setMenu(const char* str)
{
  /*attrset(COLOR_PAIR(COLOR_WHITE));
  mvaddstr(menuPos, 0, str);
  for (int i = strlen(str); i < COLS; i++) addch(' ');
  attrset(COLOR_PAIR(COLOR_BLACK));
  move(inputY, inputX);
  refresh();*/
}

void Out::setCursor(bool c)
{
  /*cursor = c;
  if (cursor) {
    curs_set(1);
  }else{
    curs_set(0);
  }
  refresh();*/
}

void Out::openLog(const char* log)
{
  logfile.open(log);
  if (logfile.is_open()) {
    openedLog = true;
  }else{
    if (initialised) addln("Error - could not open logfile");
    cerr << "Error - could not open logfile" << endl;
  }
}

void Out::closeLog()
{
  logfile.close();
  openedLog = false;
}

bool Out::getOpenedLog()
{
  return openedLog;
}
