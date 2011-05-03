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

/* The Shoot-em-up Project - A 3D game (and engine/level editor) written
 * using C++, OpenGL/GLUT, OpenAL, for windows and linux.
 *
 * Copyright (C) 2005 Trevor Dodds <trev@primatedesign.co.uk> and Ben
 * Jones <sheepyl33t@yahoo.com>
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

#ifndef _SOUNDMANAGER_
#define _SOUNDMANAGER_

#include <iostream>

#define _WIN32_

#ifndef _WIN32_

#include <termios.h>

#endif

#include <stdlib.h>
#include "AL/al.h"
#include "AL/alc.h"
//#include "AL/alut.h"
#include <vector>
#include "network.h"

// this program was inspired by
// http://www.devmaster.net/articles/openal-tutorials/lesson1.php

using namespace std;

// The number of sound buffers
// 10 non-streaming (reserved for sound fx SOUND_...)
// 10 streaming for each client - 10 clients, so that's 100 streaming
#define MAX_SOUNDS 110

// The number of sound sources
#define MAX_SOURCES 10

#define MAX_STREAMING_SOURCES 10

#define SOUND_PLAYER_HIT 0
#define SOUND_BADDIE_HIT 1
#define SOUND_EXPLOSION_ROCKET 2
#define SOUND_EXPLOSION_GRENADE 3
#define SOUND_EXPLOSION_HUGE 4
#define SOUND_SHOTGUN 5
#define SOUND_MINIGUN 6

#define CAPTURE_FORMAT AL_FORMAT_MONO16;
#define CAPTURE_FORMAT_BYTES 2
//WORKS#define CAPTURE_FREQ 11000
//WORKS2#define CAPTURE_FREQ 10000
#define CAPTURE_FREQ 20000
// note if increase this prob need to change number of bytes captured in capture()
//#define CAPTURE_FREQ 22050
//#define CAPTURE_FREQ 44100
//#define CAPTURE_BUFFER_SIZE CAPTURE_FREQ * 2 * 2 // * CAPTURE_FORMAT_BYTES
// if changing CAPTURE_BUFFER_SIZE must change CAPTURE_SAMPLES below - it's all about the time taken to fill a buffer
//WORKS#define CAPTURE_BUFFER_SIZE CAPTURE_FREQ // half a second
//WORKS2#define CAPTURE_BUFFER_SIZE CAPTURE_FREQ / 2 // quarter of a second
#define CAPTURE_BUFFER_SIZE CAPTURE_FREQ / 5 // tenth of a second (should fit into FLAG_AUDIO_SIZE)
// mono16 is 2 bytes per sample
// capture 2 seconds of samples

// CAPTURE_FREQ * numberOfSeconds per buffer
//WORKS#define CAPTURE_SAMPLES CAPTURE_FREQ / 2 // half a second
//WORKS2#define CAPTURE_SAMPLES CAPTURE_FREQ / 4 // quarter of a second
#define CAPTURE_SAMPLES CAPTURE_FREQ / 10 // tenth of a second

// if changing these watch buffer start values and MAX_SOUNDS
// number of capture buffers
#define CAPTURE_BUFFERS 3
// number of receive buffers
#define RECEIVE_BUFFERS 10

// the alGetSourcei(source[sourceId], AL_SOURCE_TYPE, &type); doesn't seem to work
// (tested on the linux implementation) so I'll do it manually instead!
#define TYPE_UNDETERMINED 0
#define TYPE_STREAMING 1
#define TYPE_STATIC 2

class SoundManager {
  private:
    // Buffers hold sound data.
    ALuint buffer[MAX_SOUNDS];

    ALCdevice *playDevice;
    ALCcontext *playContext;
    ALCdevice *captureDevice;
    ALbyte *captureBuf[CAPTURE_BUFFERS]; //[CAPTURE_BUFFER_SIZE];
    ALCint capturedSamples;
    int captureBufIdx[CAPTURE_BUFFERS], captureBufWrite, captureBufRead;
    ALbyte *receiveBuf[MAX_STREAMING_SOURCES][RECEIVE_BUFFERS];
    int receiveBufIdx[MAX_STREAMING_SOURCES][RECEIVE_BUFFERS], receiveBufWrite[MAX_STREAMING_SOURCES];
    int receiveQueue[MAX_STREAMING_SOURCES][RECEIVE_BUFFERS];
    int receiveQueueIdx[MAX_STREAMING_SOURCES];
    float receiveGain, sourceGain;
    int minQueueSize; // how much to buffer ahead?
    int sourceMap[MAX_SOURCES]; // map global id to source number one to many
    int sourceType[MAX_SOURCES];
    int nextSource;
    int receiveIdMap[MAX_STREAMING_SOURCES]; // map global id to receiveId one to one
    int nextReceiveId;

    // Sources are points emitting sound.
    ALuint source[MAX_SOURCES];

    // Store position of sources, or offset from listener for relative positions
    ALfloat position[MAX_SOURCES][3];
    
    // Listener position
    ALfloat listenerPos[3];

    vector <char*> filenames;

    void resetListenerValues();
    ALboolean loadALData(char* filename, ALuint *buf);
    void unqueuePlayedBuffers(int, int);

    int makeNewStreamingSource(int);
    int makeNewSource(int, int);
    int getStreamingSource(int);
    int getSource(int, int);
    int makeNewReceiveId(int);
    int getReceiveId(int);

  public:
    SoundManager();

    ALboolean loadSounds();

    void captureStart();
    void captureStop();
    bool capture();
    void setCaptureBufIdx(int, int);
    int getCaptureDataUnit(char*);

    bool receiveAudio(int, char*, int, float, float, float, bool);
    void updateReceiver(int, float, float, float);
    void setReceiveGain(float);
    float getReceiveGain();
    void setMinQueueSize(int);
    int getMinQueueSize();

    void setSourcePosition(ALuint source, ALfloat x, ALfloat y, ALfloat z);
    void setSourceDirection(ALuint source, ALfloat i, ALfloat j, ALfloat k);

    void setListenerPosition(ALfloat x, ALfloat y, ALfloat z);
    void setListenerVelocity(ALfloat x, ALfloat y, ALfloat z);
    void setListenerOrientation(ALfloat x, ALfloat y, ALfloat z, ALfloat i, ALfloat j, ALfloat k);

    // play a sound source at a specified position
    int play(int, int, float, float, float);
    // play a sound source relative to listener position
    int playRelative(int, int, float, float, float);
    
    void updateListener(float, float, float, float, float, float, float, float, float);
    void killALData();
};

#endif
