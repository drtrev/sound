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

#ifndef OUT
#define OUT

#include <fstream>

#define MAX_COLS 500 // There's 157 on my monitor, but allow more in case want to type more in!!
#define MAX_LINES 100 // 55 for me

class Out {
  private:
    bool initialised;
    int outputX, outputY;
    int inputX, inputY;
    int menuPos;
    int cursor;
    char outputLines[MAX_LINES][MAX_COLS];
    char inputLines[MAX_LINES][MAX_COLS];
    int outputLine, outputChar, inputLine, inputChar;

    std::ofstream logfile;
    bool openedLog;

  public:
    Out(); // constructor
    ~Out(); // destructor

    void init();
    void endWin();

    int getWidth();
    int getHeight();

    void addCh(int, int, char);
    void add(const char*); // add message
    void add(std::string);
    void add(char);
    void add(int);
    void addln(int);
    void addln(const char*);
    void addln(std::string);

    //Out& operator<<(const char*);
    //Out& operator<<(const std::string&); // need string as well as char*

    void refreshScreen(); // note this is not needed if using getCh
    int getCh();
    char get();
    char getIn(int);
    void putIn(const char*); // put some stuff into the input stream

    void scrollInput();
    void scrollOutput();

    void setMenu(const char*);

    void setCursor(bool); // turn cursor on/off

    void openLog(const char*);
    void closeLog();
    bool getOpenedLog();
};

#endif
