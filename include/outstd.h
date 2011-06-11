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

#ifndef OUTSTD
#define OUTSTD

#include <fstream>

#define MAX_COLS 500 // There's 157 on my monitor, but allow more in case want to type more in!!
#define MAX_LINES 100 // 55 for me

/**
 * The output class.
 *
 * This can be implemented using Curses, OpenGL, or simply using the standard output stream.
 */
class Out {
  private:
    bool initialised; /**< Are we initialised? */

    /** \var outputX
     *  The x coordinate used for output. */
    /** \var outputY
     *  The y coordinate used for output. */
    /** \var inputX
     *  The x coordinate of the input cursor. */
    /** \var inputY
     *  The y coordinate of the input cursor. */
    int outputX, outputY;
    int inputX, inputY;

    int menuPos; /**< The menu position. */
    int cursor;  /**< The cursor type. */
    char outputLines[MAX_LINES][MAX_COLS]; /**< Array storing the output lines. */
    char inputLines[MAX_LINES][MAX_COLS];  /**< Array storing the input lines. */

    /** \var outputLine
     *  The line number we are using for the outputLines array. \see outputLines. */
    /** \var outputChar
     *  The column number (number of characters so far) we are using for the outputLines array. \see outputLines. */
    /** \var inputLine
     *  The line number we are using for the inputLines array. \see inputLines. */
    /** \var inputChar
     *  The column number (number of characters so far) we are using for the inputLines array. \see inputLines. */
    int outputLine, outputChar, inputLine, inputChar;

    int cols, lines;

    std::ofstream logfile; /**< The filename of a logfile. */
    bool openedLog; /**< Store if log file is opened. */

    /** The constructor called with an instance of Out.
     *
     *  Declared private, but not defined, to prevent copying.
     *
     *  \param o the output instance to be copied (not used).
     */
    Out (const Out &o);

    /** The overload of the assignment operator.
     *
     *  Declared private, but not defined, to prevent assignment.
     *  \param o the output instance (not used).
     */
    Out& operator=(const Out &o);

  public:
    Out(); /**< Constructor. */
    ~Out(); /**< Destructor. */

    void init(); /**< Initialisation. */
    void endWin(); /**< End output. */

    int getWidth(); /**< Get width of output. \return width of output (number of columns). */
    int getHeight(); /**< Get height of output. \return height of output (number of rows). */

    void addCh(int y, int x, char c); /**< Add character to ouput. \param y y coordinate. \param x x coordinate. \param c the character. */
    void add(const char* msg); /**< Add character string to output. \param msg the output message. */
    void add(std::string msg); /**< Add string to output. \param msg the output message. */
    void add(char c); /**< Add character to output. \param c the character. */
    void add(int n); /**< Add integer to output. \param n the integer. */
    void addln(int n); /**< Add integer to output, followed by newline. \param n the integer. */
    void addln(const char* msg); /**< Add character string to output, followed by newline. \param msg the output message. */
    void addln(std::string msg); /**< Add string to output, followed by newline. \param msg the output message. */

    // overload stream-insertion operator
    // note that endl is actually a function in ostream, which
    // prints a newline character and flushes the output stream
    //Out& operator<<(const char*);
    //Out& operator<<(const std::string&); // need string as well as char*

    /** Refresh the output screen.
     *
     * \note This is needed if using curses.
     * However, it is not needed if using curses getch() which also performs a refresh.
     *
     * \see getCh().
     */
    void refreshScreen();

    /**
     * Get character from keyboard.
     *
     * \note If implemented using curses getch() then this also performs a refresh.
     *
     * \return the input character.
     */
    int getCh();

    /**
     * Get input into the inputLines array.
     *
     * Designed to call getIn() with getCh().
     *
     * \return the input character, which may have been modified by getIn().
     *
     * \see getCh().
     * \see getIn().
     */
    char get();

    /**
     * Get input into the inputLines array.
     *
     * \param c the character to put into the inputLines array.
     * \return the input character (may have been modified, e.g. changing carriage return and newline character to '\n').
     */
    char getIn(int c);

    /**
     * Put some stuff into the inputLines array, as if the user typed it themselves.
     *
     * \param str the string to add to the inputLines array.
     */
    void putIn(const char* str);

    void scrollInput(); /**< Shift input lines up one. */
    void scrollOutput(); /**< Shift output lines up one. */

    void setMenu(const char* str); /**< Set the menu string. \param str the menu string. */

    void setCursor(bool c); /**< Turn cursor on/off. \param c boolean representing on or off. */

    void openLog(const char* log); /**< Open log file. \param log the name of the log file. */
    void closeLog(); /**< Close log file. */
    bool getOpenedLog(); /**< Is the log file open? \return true if open, false otherwise. */
};

#endif
