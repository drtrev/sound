#ifndef CLIENTCONTROL_H
#define CLIENTCONTROL_H

#include "motlab/client.h"
#include "controller.h"
//#include "inputX11.h"
#include "inputSDL.h"
#include "motlab/dev.h"
#include "motlab/talk.h"
#include "ogg.h"

/**
 * Client control class.
 *
 * Control keyboard input, audio capture, graphics output, file transfer.
 */
class Clientcontrol : public Controller {
  private:
    Client client; /**< Underlying client instance for low level operations. */
    //InputX11 input; // doesn't seem to work in fullscreen
    InputSDL input; /**< Input object. */
    SoundDev soundDev; /**< Sound device instance. */
    //Talk talk; /**< Talk instance, used for capturing audio from the microphone. */

    /** \var keys
     * Store which keys are being held down as a bitfield. Each bit represents a
     * key (on/off).
     * \see Input */
    /** \var keysOld
     * Store which keys were being held down in the previous input loop as a
     * bitfield. */
    int keys, keysOld; // which keys are being held down, each bit represents a key, see input.h

    int myId;  /**< My client ID. */

    ogg_stream* ogg;
    int oggs;

    bool loadFile(ogg_stream &ogg, const char* filename);

  public:
    Clientcontrol();  /**< Constructor. */
    ~Clientcontrol(); /**< Destructor. */

    void init(Args &args);
    void stop();

    void go(); /**< Run through the various control loops at set frequencies. */

    /**
     * Execute the specified loop.
     *
     * \param delay the delay in microseconds before this loop should be called.
     * \param lasttime the last time that this loop was called (so it can
     * calculate if it's time to call it again yet).
     * \param loopPtr the pointer to a control loop (e.g. input, graphics,
     * sound).
     */
    void doloop(int delay, timeval &lasttime, void (Clientcontrol::*loopPtr)());

    void inputloop();    /**< The input loop. */
    void process(Unit);  /**< Process a received data unit. Part of the network loop. */
    void networkloop();  /**< Do the network sending and receiving/processing. */
    void physicsloop();  /**< Perform any local calculations, e.g. visual effects, moving objects that have no input or randomness. */
    void graphicsloop(); /**< Display graphics. */
    void soundloop();    /**< Capture audio (if required) and update playback. */
    void transferloop(); /**< Deal with file transfer. */

};

#endif
