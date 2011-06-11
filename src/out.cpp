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

#include "out.h"
#include <cstring>
#include <iostream>
#include <ncurses.h>

using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::string;
using std::strlen;

Out::Out()
{
  initialised = false;
  openedLog = false;
}

Out::~Out()
{
  endWin();

  if (openedLog) closeLog();
}

int Out::getWidth()
{
  return COLS;
}

int Out::getHeight()
{
  return LINES;
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

  initscr();             // initialize the curses library
  initialised = true;
  keypad(stdscr, TRUE);  // enable keyboard mapping
  nonl();                // tell curses not to do NL->CR/NL on output
  cbreak();              // take input chars one at a time, no wait for \n
  noecho();              // don't echo input
  nodelay(stdscr, true); // real time!
  //halfdelay(10); // slower

  if (has_colors()) {
    start_color();
    use_default_colors(); // don't use black bg unless it's default

    /*
     * Simple color assignment, often all we need.
     */
    init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_WHITE);
    init_pair(COLOR_RED, COLOR_RED, COLOR_WHITE);
    init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_WHITE);
    init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_WHITE);
    init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_WHITE);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_WHITE);
  }

  menuPos = LINES - 10;
  inputY = menuPos+1;
}

void Out::endWin()
{
  if (initialised) {
    endwin();
    initialised = false;
  }
}

/*int Out::strcount(const char* msg, const char c)
  // how many times does c occur in msg?
{
  int count = 0;
  char* ptr;

  // count up to a max of 1000 c's

  for (int i = 0; i < 1000; i++) {
    ptr = strchr(msg, c);

    if (ptr != NULL) {
      count++;

      msg = ptr + 1;
    }else break; // no more found then get out of here
  }

  return count;
}*/

void Out::addCh(int y, int x, char c)
{
  mvaddch(y, x, c);
}

void Out::add(const char* msg)
{
  if (openedLog) logfile << msg;

  if (cursor) curs_set(0);

  for (int i = 0; i < (int) strlen(msg); i++) {
    if (msg[i] != '\r') {
      mvaddch(outputY, outputX, msg[i]);
      outputX++;
      if (msg[i] == '\n' || outputX >= COLS) { // limit to line width
        // store line
        outputLines[outputLine][outputChar] = 0;
        outputChar = 0;
        outputLine++; // one day may not correspond to outputY
        if (outputLine > MAX_LINES - 1) {
          cerr << "outputLines overflowed" << endl;
          outputLine = MAX_LINES - 1;
        }
        outputY++;
        outputX = 0;

        if (outputY > menuPos - 1) scrollOutput();

      }else{
        outputLines[outputLine][outputChar] = msg[i];
        outputChar++;
        if (outputChar > MAX_COLS - 1) outputChar = MAX_COLS - 1;
      }
    }
  }
  move(inputY, inputX);
  if (cursor) curs_set(1);
  refresh();
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
  snprintf(c, 20, "%i", n);
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
  refresh();
}

int Out::getCh()
{
  // this also performs refresh
  return getch();
}

char Out::get()
{
  return getIn(getch());
}

void Out::putIn(const char* str)
{
  for (int i = 0; i < (int) strlen(str); i++) getIn(str[i]);
}

char Out::getIn(int c)
{
  // c could be \n if called using putIn
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

  return c;
}

void Out::scrollInput()
{
  // shift input lines up one
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

  refresh();
}

void Out::scrollOutput()
{
  // shift output lines up one
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
  refresh();
}

void Out::setMenu(const char* str)
{
  attrset(COLOR_PAIR(COLOR_WHITE));
  mvaddstr(menuPos, 0, str);
  for (int i = strlen(str); i < COLS; i++) addch(' ');
  attrset(COLOR_PAIR(COLOR_BLACK));
  move(inputY, inputX);
  refresh();
}

void Out::setCursor(bool c)
{
  cursor = c;
  if (cursor) {
    curs_set(1);
  }else{
    curs_set(0);
  }
  refresh();
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
