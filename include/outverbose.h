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

#ifndef OUTVERBOSE
#define OUTVERBOSE

#include "outstd.h"

/**
 * The enum storing the three verbose values, VERBOSE_QUIET, VERBOSE_NORMAL and VERBOSE_LOUD.
 *
 * \see verbose.
 * \see outVerbosity.
 * \see checkVerbosity().
 */
enum verboseEnum { VERBOSE_QUIET, VERBOSE_NORMAL, VERBOSE_LOUD };

// always use error stream for operator overloads
//enum streamEnum { STREAM_STDOUT, STREAM_STDERR };

/**
 * The Outverbose class.
 *
 * This extends the output class to deal with stream operators, error streams, and verbosity.
 */
class Outverbose : public Out {
  private:
    /** \var verbose
     *  The user's setting for verbosity. For example, if they specify -v on the command line,
     *  this can be set to VERBOSE_LOUD, and a -q can be used to set this to VERBOSE_QUIET.
     *  If they don't specify anything, then this can default to VERBOSE_NORMAL.
     *
     *  \see verboseEnum.
     *  \see checkVerbosity().
     */
    /** \var outVerbosity
     *  The programmer's verbosity setting. For example, error messages can be output using VERBOSE_LOUD,
     *  and minor system information with VERBOSE_QUIET. Everything else can be output with VERBOSE_NORMAL.
     *  The value can be set using the stream operator, e.g. out << VERBOSE_LOUD << "Fatal error.\n";
     *
     *  \see Outverbose& operator<<(const verboseEnum);
     *  \see verboseEnum.
     *  \see checkVerbosity().
     */
    verboseEnum verbose, outVerbosity;
    //streamEnum err;

    /**
     * Check if the output should be displayed, based on the user's setting (verbose) and the programmer's setting v,
     * which would usually be set to outVerbosity. If the programmer's setting is louder or equal to the user's setting,
     * then the output should be displayed, (i.e. if (v > 1 - verbose) return true; else return false; ).
     *
     * \param v the programmer's verbosity setting, which should be set to outVerbosity under normal circumstances.
     * \return true if output should be displayed, or false if it should be suppressed.
     */
    bool checkVerbosity(verboseEnum v);

  public:
    Outverbose(); /**< Constructor. */
    ~Outverbose(); /**< Destructor. */

    void setVerbosity(verboseEnum v); /**< Set verbosity. \param v the user's verbosity level. \see verbose. */

    /**
     * Ouput the verbosity level of the parameter in a human readable form (e.g. Quiet, Normal, Loud).
     *
     * \param v the verbosity level to output.
     * \see verboseEnum.
     */
    void outputVerbosity(verboseEnum v);

    void add(const char* msg, verboseEnum v); /**< Output a message. \param msg the character string to output. \param v the programmer's verbosity level. */
    void add(std::string msg, verboseEnum v); /**< Output a message. \param msg the string to output. \param v the programmer's verbosity level. */
    void add(char c, verboseEnum v); /**< Output a character. \param c the character to output. \param v the programmer's verbosity level. */
    void add(int i, verboseEnum v); /**< Output an integer. \param i the integer to output. \param v the programmer's verbosity level. */
    void addln(int i, verboseEnum v); /**< Output an integer and newline. \param i the integer to output. \param v the programmer's verbosity level. */
    void addln(const char* msg, verboseEnum v); /**< Output a message and newline. \param msg the character string to output. \param v the programmer's verbosity level. */
    void addln(std::string msg, verboseEnum v); /**< Output a message and newline. \param msg the string to output. \param v the programmer's verbosity level. */

    void adderr(const char* msg, verboseEnum v); /**< Output a message, and copy to stderr. \param msg the character string to output. \param v the programmer's verbosity level. */
    void adderr(std::string msg, verboseEnum v); /**< Output a message, and copy to stderr. \param msg the string to output. \param v the programmer's verbosity level. */
    void adderr(char c, verboseEnum v); /**< Output a character, and copy to stderr. \param c the character to output. \param v the programmer's verbosity level. */
    void adderr(int i, verboseEnum v); /**< Output an integer, and copy to stderr. \param i the integer to output. \param v the programmer's verbosity level. */
    void adderrln(int i, verboseEnum v); /**< Output an integer and newline, and copy to stderr. \param i the integer to output. \param v the programmer's verbosity level. */
    void adderrln(const char* msg, verboseEnum v); /**< Output a message and newline, and copy to stderr. \param msg the character string to output. \param v the programmer's verbosity level. */
    void adderrln(std::string msg, verboseEnum v); /**< Output a message and newline, and copy to stderr. \param msg the character string to output. \param v the programmer's verbosity level. */

    Outverbose& operator<<(const verboseEnum v); /**< Change the programmer's verbosity setting. \param v the verbosity setting. */
    Outverbose& operator<<(const char* str); /**< Output a string. \param str the string to output. */
    Outverbose& operator<<(const std::string& str); /**< Output a string. \param str the string to output. */
    Outverbose& operator<<(const char c); /**< Output a character. \param c the character to output. */
    Outverbose& operator<<(const int i); /**< Output an integer. \param i the integer to output. */

    using Out::addln; // make parent functions available here, since they've been overloaded
    using Out::add;
};

#endif
