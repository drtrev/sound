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

/* This class was adapted from DevMaster.net's Open AL tutorial lesson 8
 * by Jesse Maurais and lesson 9 by Spree Tree
 * http://www.devmaster.net/articles/openal-tutorials/lesson8.php
 * http://www.devmaster.net/articles/openal-ogg-file/
 *
 * Jesse Maurais - jesse_maurais@hotmail.com
 * Spree Tree - SpreeTree@hotmail.com
 * Port to Linux by Lee Trager - nukem@users.xeroprj.org
 */

#ifndef oggplayer
#define oggplayer

#include <string>
#include <iostream>
using namespace std;

#include <AL/al.h>
#include <AL/alc.h> // the OpenAL context API
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include "types.h"

// Struct that contains the pointer to our file in memory
struct SOggFile
{
  char* dataPtr;   // Pointer to the data in memoru
  int dataSize;   // Sizeo fo the data

  int dataRead;  // How much data we have read so far
};

// 4096 * 4 = 16 384
// 16384 / 128000 = 0.128 seconds stored in buffer for 128 kbit/s
//#define OGG_BUFFER_SIZE (4096 * 4)
//#define OGG_BUFFER_SIZE (4096 * 8) // works well but skips when changing windows and stuff
//#define OGG_BUFFER_SIZE (4096 * 16)
// 44100 * 2 = 88200
// 8820 = 0.1 secs
#define OGG_BUFFER_SIZE 17640


#define OGG_NUM_BUFFERS 2
//#define OGG_NUM_BUFFERS 4

class ogg_stream
{
  public:

    ogg_stream();
    void open_inmemory(string path);
    void open(string path);
    void release();
    void display();
    string getComment(char*);
    string getTitle();
    string getArtist();
    string getGenre();
    string getDate();
    string getAlbum();
    string getTracknumber();
    bool playback_binaural();
    bool playback(bool);
    bool playing();
    void setSourceId(int);
    int getSourceId();
    void setPosition(float, float, float);
    void getPosition(float&, float&, float&);
    float getX();
    float getY();
    float getZ();
    void setX(float);
    void setY(float);
    void setZ(float);
    void setSpeed(float, float, float);
    void getSpeed(float&, float&, float&);
    /*void updateVelocity();
    void setSpeedX(float);
    void setSpeedY(float);
    void setSpeedZ(float);*/
    float getSpeedX();
    float getSpeedY();
    float getSpeedZ();
    void setPaused(bool);
    bool getPaused();
    void setMoving(bool);
    bool getMoving();
    void setHoming(bool);
    bool getHoming();
    void getTarget(float&, float&, float&);
    void setTarget(float, float, float);
    void setFriction(bool);
    bool getFriction();
    bool update();
    void move(double);
    double time_total();
    double tell();
    void seek(double);

    bool getSourceInitialised();

    void moveme(double);

    void setPitch(float, bool); // binaural test

    //void killAL();

  protected:

    void initSource();
    bool stream(ALuint buffer);
    int unqueuePlayedBuffers();
    void empty();
    int check(string);
    string errorString(int code);

  private:

    // sound manager stuff
    //ALCdevice *playDevice;
    //ALCcontext *playContext;

    FILE*           oggFile;          // For playing directly from file
    SOggFile        oggMemoryFile;    // Data on the ogg file in memory
    ov_callbacks    vorbisCallbacks;  // callbacks used to read the file from mem

    OggVorbis_File  oggStream;
    vorbis_info*    vorbisInfo;
    vorbis_comment* vorbisComment;

    ALuint buffers[OGG_NUM_BUFFERS];
    ALuint source;
    ALenum format;
    ALuint bufferIdx;

    bool sourceInitialised;

    int sourceId; // the source that it is bound to

    struct Props {
      sound::Vector pos, oldPos, speed, targ, accel, oldAccel;
      bool moving, homing, frictionon, paused;
      float pitch, power, friction, minSpeed;

      Props() : moving(true), homing(false), frictionon(true), paused(false), pitch(1), power(800), friction(400), minSpeed(0.001) {}
    } props;

    float synclimit;

    //float x, y, z;
    //float speedX, speedY, speedZ;
    //bool paused;
    //bool moving; // some kind of movement animation has been set up
    //bool homing; // homing in on centre
    //float targX, targY, targZ;
    //bool friction; // apply friction

    //float pitch; // binaural test
};


#endif
