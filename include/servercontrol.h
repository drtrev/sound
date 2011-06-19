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

#ifndef SERVERCONTROL_H
#define SERVERCONTROL_H

#include "controller.h"
#include "inputSDL.h"
#include "motlab/server.h"

/**
 * Servercontrol class.
 *
 * Deals with the server, holds the shared state of the environment.
 */

class Servercontrol : public Controller {
  private:
    Server server; /**< Server instance. */

    InputSDL input; /**< Input instance. */

    timeval lastSent; /**< Time last transmitted a state update. */

    int keyset[MAX_CLIENTS]; /**< Keys received from clients. */
    int keys; /**< Keys pressed by server user. */

    // TODO use geo::Vector
    struct Vector {
      float x, y, z;
      Vector() : x(0), y(0), z(0) {}
    } oldposition[MAX_CLIENTS];

  public:
    Servercontrol(); /**< Constructor. */
    ~Servercontrol(); /**< Destructor. */

    /**
     * Initialisation.
     *
     * \param port the port number to use.
     * \param readPath the path to read files from (when using Motlab data transfer).
     * \param writePath the path to write files to (when using Motlab data transfer).
     * \param mapfile the file containing the map data.
     * \param verbosity the user's verbosity level.
     * \param ip the IP address to connect to (here because client and server share init prototype.
     * \param dontGrab don't grab the keyboard.
     * \param fullscreen run in fullscreen mode.
     * \param polygonMax the maximum number of polygons to load.
     */
    void init(Args &args);
    void stop(); /**< End everything. */

    void go(); /**< Main loop. Call input, network and physics loops. */

    //void doloop(int, timeval&, void (Servercontrol::*)());

    /**
     * Execute the specified loop.
     *
     * \param delay the delay in microseconds before this loop should be called.
     * \param lasttime the last time that this loop was called (so it can
     * calculate if it's time to call it again yet).
     * \param loopPtr the pointer to a control loop (e.g. input, graphics,
     * sound).
     */
    void doloop(int delay, timeval &lasttime, void (Servercontrol::*loopPtr)());

    void inputloop();      /**< The input loop. */
    void sendStatus(int);  /**< Send current shared state to new client. */
    void process(Unit);    /**< Process a received data unit. Part of the network loop. */
    void networkloop();    /**< Do the network sending and receiving/processing. */
    void physicsloop();    /**< Perform any movement calculations. */
    void graphicsloop();   /**< Display graphics. */

};

#endif
