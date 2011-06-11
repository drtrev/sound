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

//#include <AL/alut.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <signal.h>
#include <sys/time.h> // for gettimeofday
#include <GL/glut.h>
#include <getopt.h>

#include "soundmanager.h"
#include "ogg.h"
#include "menu.h"

// 60 FPS expected, so 1 / 60 seconds per frame
#define SECS_PER_FRAME 0.0166666667

#define PI 3.14159265

using namespace std;

SoundManager soundMan;

// for frustum, set in init
float rightFrustumBorder = 0.0;
float nearPlane = 0.1, farPlane = 1000.0;

int windowWidth = 0, windowHeight = 0, windowCentreX = 0, windowCentreY = 0;
int window = -1;
enum { WINDOW_MODE_NORMAL, WINDOW_MODE_FULLSCREEN, WINDOW_MODE_GAMEMODE_1024, WINDOW_MODE_GAMEMODE_1280 };
int windowMode = WINDOW_MODE_NORMAL;

// camera
struct Camera {
  float angleX, angleY, angleZ;
  float x, y, z;
  float speed;
};
Camera cam;

// mouse
int mouseX = 0, mouseY = 0, mouseMoveX = 0, mouseMoveY = 0;
int mouseStoreX = 0, mouseStoreY = 0;
float mouseSensitivity = 0.5;
bool mouseLookEnabled = false, mouseInvert = false;
bool pressRightButton = false;
bool warped = false;

// keyboard
bool pressLeft = false, pressRight = false, pressUp = false, pressDown = false;

// files
#define SOURCE_MAX 2

// auto DJ
bool autoDj = false;
int autoDjSource = -1;

//int numFiles = 0
int sourceSelected = -1;
float arrowRot = 0.0; // arrow rotation
ogg_stream ogg[SOURCE_MAX];

// binaural
struct Binaural {
  bool enabled;
  float x;
  float y;
  float z;
  struct timeval timeStart;
};
Binaural binaural;

double phaseDiff = 0;
double lastPhaseDiff = 0;

// menu
Menu sourceMenu[2];
Menu quickDjMenu;


// playlist
struct Playlist {
  string file;
  string title;
  int source;
};
#define PLAYLIST_MAX 100
// also note BUTTON_MAX
Playlist playlist[PLAYLIST_MAX];
Playlist playlistOriginal[PLAYLIST_MAX];
int playlistNum = 0;
int playlistDrag = -1;
int playlistReset = 0;

Menu playlistMenu;
Menu playlistMenuOptions;
Menu playlistMenuTime;

bool scrolling = false;


// messages
struct Message {
  string text;
  struct timeval time;
  bool active;
};
#define MESSAGE_MAX 12
Message message[MESSAGE_MAX];
int messageNum = 0;
float messageDuration = 10.0;
int messageWrapped = 0; // number of wrapped lines


// timing
struct timeval timeLast;
struct timeval timeCurrent;
struct timeval timeSince;
struct timeval timePress; // for double click
#define DOUBLE_CLICK_DELAY 500000 // micro seconds for double click



// lighting
// if fourth parameter is 0, then the light is directional (assumed to be facing the origin from its position)
// - there is no attenuation

// produces a different shade on every side of a max white cube at origin
GLfloat lightCoords0[] = { -30.0, 50.0, 20.0, 0 };
GLfloat ambient0[] = { 0.3, 0.3, 0.3, 1.0 };
GLfloat diffuse0[] = { 0.3, 0.3, 0.3, 1.0 };
GLfloat specular0[] = { 0.0, 0.0, 0.0, 1.0 };

GLfloat lightCoords1[] = { 35.0, -40.0, -20.0, 0 };
GLfloat ambient1[] = { 0.3, 0.3, 0.3, 1.0 };
GLfloat diffuse1[] = { 0.3, 0.3, 0.3, 1.0 };
GLfloat specular1[] = { 0.0, 0.0, 0.0, 1.0 };

GLfloat lightCoords2[] = { 0.0, 10.0, 0.0, 1 };
GLfloat ambient2[] = { 0.3, 0.3, 0.3, 1.0 };
GLfloat diffuse2[] = { 0.3, 0.3, 0.3, 1.0 };
GLfloat specular2[] = { 0.0, 0.0, 0.0, 1.0 };

//
// operations
//
void messageAdd(const char* c)
{
  if (messageNum > MESSAGE_MAX - 1) {
    // shift down
    for (int i = 0; i < MESSAGE_MAX - 1; i++) message[i] = message[i+1];
    messageNum = MESSAGE_MAX - 1;
  }
  string str = c;
  message[messageNum].text = str;
  struct timeval timeCur;
  gettimeofday(&timeCur, NULL);
  // time is too big to fit a float
  message[messageNum].time.tv_sec = timeCur.tv_sec;
  message[messageNum].time.tv_usec = timeCur.tv_usec;
  message[messageNum].active = true;
  messageNum++;
}

void messageHelp()
  // display help
{
  messageAdd("Quick controls:");
  messageAdd("   a - toggle auto DJ, n - auto play next");
  messageAdd("   s - shuffle playlist, u - uncross all");
  messageAdd("   y - tidy playlist");
  messageAdd("The following act on the selected source:");
  messageAdd("   z - rewind to start, x - rewind 10 secs");
  messageAdd("   c - pause, v - forward 10 secs, b - skip");
  messageAdd("   g - centre, o - toggle home, m - toggle move");
  messageAdd("   f - friction, r - store point, t - rewind to point");
  messageAdd("General controls:");
  messageAdd("   h - help (display this message), l - reload playlist");
  messageAdd("   0 - select source 0, 1 - select source 1");
}

int playlistLoad(Playlist play[])
{
  int playNum = 0;

  ifstream playlistFile("playlist");
  string str;
  if (playlistFile.is_open()) {

    while (!playlistFile.eof()) {

      getline(playlistFile, str);
      if (!playlistFile.eof() && playNum > PLAYLIST_MAX - 1) {
        cerr << "Playlist full!" << endl;
        break;
      }

      if (!playlistFile.eof()) {
        play[playNum].file = str;
        getline(playlistFile, str);
        if (!playlistFile.eof()) {
          play[playNum].title = str;
          playNum++;
        }
      }

    }

  }else messageAdd("Error opening playlist file!");

  return playNum;
}

void playlistReload()
{
  Playlist playNew[PLAYLIST_MAX];
  int playNum = playlistLoad(playNew);
  for (int i = 0; i < playNum || i < playlistNum; i++) {
    if (playlistOriginal[i].file != playNew[i].file) {
      if (i > playlistNum - 1) {
        // new song
        if (playlistNum > PLAYLIST_MAX - 1) messageAdd("Could not add to playlist: Playlist full!");
        else{
          playlist[playlistNum].title = playNew[i].title;
          playlist[playlistNum].file = playNew[i].file;
          playlistMenu.addButton(BUTTON_NONE, playlist[playlistNum].title, 5, playlistNum * 15 + 5);
          playlistNum++;
          messageAdd("Added to playlist");
        }
      }else{
        // load the lot

        // find existing sources
        string file0 = "", file1 = "";
        int playing0 = 0, playing1 = 0;
        for (int j = 0; j < playlistNum; j++) {
          if (playlist[j].source == 0) {
            file0 = playlist[j].file;
            playing0 = j;
          }
          if (playlist[j].source == 1) {
            file1 = playlist[j].file;
            playing1 = j;
          }
        }

        // copy new playlist
        for (int j = 0; j < playNum; j++) {
          playlist[j].title = playNew[j].title;
          playlist[j].file = playNew[j].file;
          playlistOriginal[j].title = playNew[j].title;
          playlistOriginal[j].file = playNew[j].file;
        }
        playlistNum = playNum;

        // redo the menu
        playlistMenu.removeButtons();
        for (int i = 0; i < playlistNum; i++) {
          // this also sets them as unplayed
          playlistMenu.addButton(BUTTON_NONE, playlist[i].title, 5, i * 15 + 5);
        }

        // load current song again if paused
        playlistReset = 0;
        if (ogg[0].getPaused()) {
          playlistReset++;
          ogg[0].release();
        }
        if (ogg[1].getPaused()) {
          playlistReset++;
          ogg[1].release();
        }

        // if playlist has shrunk and playing sources are out of range, move them
        if (playing0 > playlistNum - 1) {
          playlist[playing0].source = -1; // not used
          if (playing1 == 0) playing0 = 1;
          else playing0 = 0;
          playlist[playing0].source = 0;
        }
        if (playing1 > playlistNum - 1) {
          playlist[playing1].source = -1; // not used
          if (playing0 == 0) playing1 = 1;
          else playing1 = 0;
          playlist[playing1].source = 1;
        }
        // if sources are different, load current song for them
        //playlistReset = 0;
        //if (playlist[playing0].file != file0) playlistReset++;
        //if (playlist[playing1].file != file1) playlistReset++;

        messageAdd("Loaded new playlist");
        break;
      }
    }
  }
}

void sourceHome()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].setHoming(!ogg[sourceSelected].getHoming());
    if (ogg[sourceSelected].getHoming()) ogg[sourceSelected].setTarget(0, 0, -50);
  }
}

void sourceFlyCentre()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].setTarget(0, 0, -50);
    ogg[sourceSelected].setMoving(true);
  }
}

void sourceFlyLeft()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].setTarget(-1000, 0, -50);
    ogg[sourceSelected].setMoving(true);
  }
}

void sourceFlyRight()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].setTarget(1000, 0, -50);
    ogg[sourceSelected].setMoving(true);
  }
}

void sourceFriction()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].setFriction(!ogg[sourceSelected].getFriction());
  }
}

void sourceMove()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].setMoving(!ogg[sourceSelected].getMoving());
    if (ogg[sourceSelected].getMoving())
      ogg[sourceSelected].setSpeed(1.2, 0, 1.5); // 1.2, 0, 1.5
    else
      ogg[sourceSelected].setSpeed(0.0, 0.0, 0.0);
  }
}

void sourceSkip()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].release();
  }
}

void sourceCentre()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].setPosition(0, 0, -20);
    ogg[sourceSelected].setSpeed(0, 0, 0);
  }
}

int getSourceNearestCentre()
{
  float dist0 = 0, dist1 = 0;
  dist0 = abs(ogg[0].getX());
  dist1 = abs(ogg[1].getX());
  if (dist0 < dist1) return 0;
  return 1;
}

void sourceNext()
{
  // find nearest source to the centre, that's the current one.
  // then auto DJ to the second
  autoDjSource = getSourceNearestCentre();
  if (autoDjSource) {
    autoDjSource = 0;
    if (ogg[0].getX() < -500) ogg[0].setX(-500);
    if (ogg[0].getX() > 500) ogg[0].setX(500);
    ogg[1].setTarget(1000, 0, -50);
  }else{
    autoDjSource = 1;
    if (ogg[1].getX() < -500) ogg[1].setX(-500);
    if (ogg[1].getX() > 500) ogg[1].setX(500);
    ogg[0].setTarget(-1000, 0, -50);
  }
  ogg[autoDjSource].setPaused(false);
  ogg[autoDjSource].setHoming(true);
  ogg[autoDjSource].setFriction(true);
  ogg[autoDjSource].setMoving(true);
  ogg[autoDjSource].setTarget(0, 0, -50);

  ogg[!autoDjSource].setHoming(false);
  ogg[!autoDjSource].setFriction(true);
  ogg[!autoDjSource].setMoving(true);

  autoDj = true;
}

void sourceEnqueue(int x, int y)
  // line source up to be played next
{
  int button = playlistMenu.checkOverButtonNum(x, y);
  
  if (button > -1 && playlist[button].source < 0) {
    // if clicked on a button and it's not already a source

    // choose a source to attach it to
    int source = 1;
    if (ogg[0].getPaused()) source = 0;

    // uncross it
    playlistMenu.setPlayed(button, false);

    // store a copy
    Playlist temp = playlist[button];

    // find current source
    for (int i = 0; i < playlistNum; i++) {
      if (playlist[i].source == source) {
        if (i < button) {
          // shift them down
          for (int j = button; j > i; j--) {
            playlist[j] = playlist[j-1];
            if (playlist[j].source == source) playlist[j].source = -1;
          }
        }else{
          // shift them up
          for (int j = button; j < i; j++) {
            playlist[j] = playlist[j+1];
            if (playlist[j].source == source) playlist[j].source = -1;
          }
        }
        playlist[i] = temp;
        playlist[i].source = source;
        break;
      }
    }

    // clear menu
    playlistMenu.removeButtons();
    for (int i = 0; i < playlistNum; i++) {
      // this also sets them as unplayed
      playlistMenu.addButton(BUTTON_NONE, playlist[i].title, 5, i * 15 + 5);
      if (playlist[i].source == -2) playlistMenu.setPlayed(i, true);
    }
    ogg[source].release();
    playlistReset++; // don't skip the track

  }

}

void sourcePause()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].setPaused(!ogg[sourceSelected].getPaused());
  }
}

void sourceRewindToStart()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].seek(0); // rewind
  }
}

void sourceRewind()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].seek(ogg[sourceSelected].tell() - 10); // rewind 10 secs
  }
}

void sourceFastforward()
{
  if (sourceSelected > -1) {
    ogg[sourceSelected].seek(ogg[sourceSelected].tell() + 10); // fast forward 10 secs
  }
}

void playlistTidy()
  // move played songs to the top, move sources to the next two
{
  if (playlistNum > 1) {
    Playlist temp[playlistNum]; // store old
    for (int i = 0; i < playlistNum; i++) temp[i] = playlist[i];

    int numLeft = playlistNum;

    int idx = 0; // idx of new playlist

    // move played tracks to the top
    for (int i = 0; i < numLeft; i++) {
      // find matching menu item for temp[i]
      for (int j = idx; j < playlistNum; j++) {
        if (playlistMenu.getButtonText(j) == temp[i].title && playlistMenu.getPlayed(j)) {
          playlist[idx] = temp[i];
          playlistMenu.changeButton(idx, playlist[idx].title);
          playlistMenu.setPlayed(idx, true);
          if (playlist[idx].source > -1) {
            // it should only be being played if everything has been played
            playlistMenuOptions.moveButton(playlist[idx].source, 5, idx * 15 + 5);
            playlistMenuTime.moveButton(playlist[idx].source, 5, idx * 15 + 5);
          }
          numLeft--;
          for (int k = i; k < numLeft; k++) temp[k] = temp[k+1]; // shift array up
          idx++; // added to new playlist
          i--; // removed one from temp
          break;
        }
      }
    }
    
    // find current sources and take them from temp
    for (int i = 0; i < numLeft; i++) {
      if (temp[i].source > -1) {
        playlist[idx] = temp[i];
        playlistMenu.changeButton(idx, playlist[idx].title);
        playlistMenu.setPlayed(idx, false);
        playlistMenuOptions.moveButton(playlist[idx].source, 5, idx * 15 + 5);
        playlistMenuTime.moveButton(playlist[idx].source, 5, idx * 15 + 5);
        numLeft--;
        for (int j = i; j < numLeft; j++) temp[j] = temp[j+1]; // shift array up
        idx++; // added to new playlist
        i--; // removed one from temp
      }
    }

    // if one source is paused and it is above the other source, swap them
    if (idx > 1) {
      if (playlist[idx - 2].source > -1 && playlist[idx - 1].source > -1 && ogg[playlist[idx - 2].source].getPaused()) {
        Playlist swap;
        swap = playlist[idx - 2];
        playlist[idx - 2] = playlist[idx - 1];
        playlist[idx - 1] = swap;
        playlistMenu.changeButton(idx - 2, playlist[idx - 2].title);
        playlistMenu.setPlayed(idx - 2, false);
        playlistMenuOptions.moveButton(playlist[idx - 2].source, 5, (idx - 2) * 15 + 5);
        playlistMenuTime.moveButton(playlist[idx - 2].source, 5, (idx - 2) * 15 + 5);

        playlistMenu.changeButton(idx - 1, playlist[idx - 1].title);
        playlistMenu.setPlayed(idx - 1, false);
        playlistMenuOptions.moveButton(playlist[idx - 1].source, 5, (idx - 1) * 15 + 5);
        playlistMenuTime.moveButton(playlist[idx - 1].source, 5, (idx - 1) * 15 + 5);
      }
    }

    for (int i = 0; i < numLeft; i++) {
      playlist[i+idx] = temp[i];
      playlistMenu.changeButton(i+idx, playlist[i+idx].title);
      if (playlist[i+idx].source > -2) playlistMenu.setPlayed(i+idx, false);
      else playlistMenu.setPlayed(i+idx, true);
    }
  }
}

void playlistShuffle()
  // shuffle the playlist, with played on top, then currently playing
{
  if (playlistNum > 1) {
    Playlist temp[playlistNum]; // store old
    for (int i = 0; i < playlistNum; i++) temp[i] = playlist[i];

    int numLeft = playlistNum;

    int idx = 0; // idx of new playlist

    // move played tracks to the top
    for (int i = 0; i < numLeft; i++) {
      // find matching menu item for temp[i]
      for (int j = idx; j < playlistNum; j++) {
        if (playlistMenu.getButtonText(j) == temp[i].title && playlistMenu.getPlayed(j)) {
          playlist[idx] = temp[i];
          playlistMenu.changeButton(idx, playlist[idx].title);
          playlistMenu.setPlayed(idx, true);
          if (playlist[idx].source > -1) {
            // it should only be being played if everything has been played
            playlistMenuOptions.moveButton(playlist[idx].source, 5, idx * 15 + 5);
            playlistMenuTime.moveButton(playlist[idx].source, 5, idx * 15 + 5);
          }
          numLeft--;
          for (int k = i; k < numLeft; k++) temp[k] = temp[k+1]; // shift array up
          idx++; // added to new playlist
          i--; // removed one from temp
          break;
        }
      }
    }
    
    // find current sources and take them from temp
    for (int i = 0; i < numLeft; i++) {
      if (temp[i].source > -1) {
        playlist[idx] = temp[i];
        playlistMenu.changeButton(idx, playlist[idx].title);
        playlistMenu.setPlayed(idx, false);
        playlistMenuOptions.moveButton(playlist[idx].source, 5, idx * 15 + 5);
        playlistMenuTime.moveButton(playlist[idx].source, 5, idx * 15 + 5);
        numLeft--;
        for (int j = i; j < numLeft; j++) temp[j] = temp[j+1]; // shift array up
        idx++; // added to new playlist
        i--; // removed one from temp
      }
    }

    // if one source is paused and it is above the other source, swap them
    if (idx > 1) {
      if (playlist[idx - 2].source > -1 && playlist[idx - 1].source > -1 && ogg[playlist[idx - 2].source].getPaused()) {
        Playlist swap;
        swap = playlist[idx - 2];
        playlist[idx - 2] = playlist[idx - 1];
        playlist[idx - 1] = swap;
        playlistMenu.changeButton(idx - 2, playlist[idx - 2].title);
        playlistMenu.setPlayed(idx - 2, false);
        playlistMenuOptions.moveButton(playlist[idx - 2].source, 5, (idx - 2) * 15 + 5);
        playlistMenuTime.moveButton(playlist[idx - 2].source, 5, (idx - 2) * 15 + 5);

        playlistMenu.changeButton(idx - 1, playlist[idx - 1].title);
        playlistMenu.setPlayed(idx - 1, false);
        playlistMenuOptions.moveButton(playlist[idx - 1].source, 5, (idx - 1) * 15 + 5);
        playlistMenuTime.moveButton(playlist[idx - 1].source, 5, (idx - 1) * 15 + 5);
      }
    }

    int chosen = -1;
    for (int i = idx; i < playlistNum; i++) {
      chosen = rand() % numLeft;
      playlist[i] = temp[chosen];
      playlistMenu.changeButton(i, playlist[i].title);
      if (playlist[i].source > -1) {
        playlistMenuOptions.moveButton(playlist[i].source, 5, i * 15 + 5);
        playlistMenuTime.moveButton(playlist[i].source, 5, i * 15 + 5);
      }
      if (playlist[i].source > -2) playlistMenu.setPlayed(i, false);
      else playlistMenu.setPlayed(i, true);
      numLeft--;
      for (int j = chosen; j < numLeft; j++) temp[j] = temp[j+1];
    }
  }
}

void playlistUncrossAll()
{
  for (int i = 0; i < playlistNum; i++) {
    if (playlist[i].source == -2) playlist[i].source = -1;
    playlistMenu.setPlayed(i, false);
  }
}


//
// frame
//
void toTarget(float &var, float targ, float amount)
  // move var towards targ, by amount
{
  if (var < targ) {
    var += abs(amount);
    if (var > targ) var = targ;
  }
  if (var > targ) {
    var -= abs(amount);
    if (var < targ) var = targ;
  }
}

float accelDecel(float a, float targ, float &speed, float accel, float maxSpeed)
  // moves the value of 'a' towards 'targ' by accelerating then decelerating smoothly
  // (speed needs to be remembered, accel is rate of speed change)
  // set maxSpeed to 0 for no limit
{
  // when target is considered reached
  if (speed > -1 && speed < 1 && a > targ - 1 && a < targ + 1) {
    speed = 0.0;
    return a;
  }

  float start = 0;
  // sum of a sequence formula: ((a1 + an)n) / 2
  // n is length of sequence, which is speed / accel
  if (speed > 0) start = a - ((speed + accel) * (speed / accel)) / 2.0;
  else start = a + ((speed - accel) * (speed / accel)) / 2.0;

  // find half way point
  float half = (start + targ) / 2.0;

  // if point is before half way, increase speed. If it's after, decrease speed
  // if it will cross half way, keep speed the same
  if (a > half && a + speed > half) speed -= accel;
  else if (a < half && a + speed < half) speed += accel;

  if (maxSpeed > 0) {
    if (speed > maxSpeed) speed = maxSpeed;
    if (speed < -maxSpeed) speed = -maxSpeed;
  }
  
  a += speed;

  return a;
}

float degToRad(float angle)
{
  return angle / 180.0 * PI;
}

float radToDeg(float angle)
{
  return angle / PI * 180.0;
}

float getTimeSinceLastFrame()
{
  gettimeofday(&timeCurrent, NULL);
  timeSince.tv_sec = timeCurrent.tv_sec - timeLast.tv_sec;
  timeSince.tv_usec = timeCurrent.tv_usec - timeLast.tv_usec;

  if (timeSince.tv_usec < 0) {
    timeSince.tv_sec--;
    timeSince.tv_usec += 1000000;
  }

  timeLast.tv_sec = timeCurrent.tv_sec;
  timeLast.tv_usec = timeCurrent.tv_usec;

  return timeSince.tv_sec + timeSince.tv_usec / 1000000.0;
}


bool playFile(ogg_stream &ogg, const char* filename, float px, float py, float pz, float sx, float sy, float sz)
{
  try {
    ogg.open_inmemory(filename);
  }catch(string error){
    char c[50];
    snprintf(c, 50, "Could not open file: %s", filename);
    messageAdd(c);
    return false; // failure
  }

  ogg.display();
  ogg.setPosition(px, py, pz);
  ogg.setSpeed(sx, sy, sz);
  //ogg.updateVelocity();

  try {
    if(!ogg.playback(false))
      throw string("Ogg refused to play.");

  }
  catch(string error)
  {
    cout << error << endl;
    char c[50];
    snprintf(c, 50, "Error: %s", error.c_str());
    messageAdd(c);
    return false; // failure
  }

  // TODO quick hack for binaural... it starts paused
  if (!binaural.enabled) {
    if (!ogg.playing()) {
      cout << "Releasing ogg file and exiting..." << endl;
      ogg.release();
      exit(1);
    }
  }

  return true; // success
}


bool playlistNext(ogg_stream &ogg)
{
  // find next unbound item in playlist after this source
  // so first find this source
  int pos = -1, play = -1;
  for (int i = 0; i < playlistNum; i++) {
    if (playlist[i].source == ogg.getSourceId()) {
      if (playlistReset > 0) { // a new playlist was loaded
        playlistReset--;
        playlist[i].source = -1;
        pos = i - 1;
      }else{
        playlist[i].source = -2; // played
        playlistMenu.setPlayed(i, true);
        pos = i;
      }
    }
  }
  // now go through all the playlist items after this until there's an unbound one
  for (int i = pos + 1; i < playlistNum; i++) {
    if (playlist[i].source == -1) {
      playlist[i].source = ogg.getSourceId();
      play = i;
      break;
    }
  }

  // if play is -1 then playlist is finished
  if (play > -1) {
    playlistMenuOptions.moveButton(ogg.getSourceId(), 5, play * 15 + 5);
    playlistMenuTime.moveButton(ogg.getSourceId(), 5, play * 15 + 5);
    float x = 0, y = 0, z = 0;
    ogg.getPosition(x, y, z);
    playFile(ogg, playlist[play].file.c_str(), x, y, z, 0, 0, 0);
    return true;
  }else{
    return false;
  }
}

int sourceProcess(float timeSecs, ogg_stream &ogg)
  // returns amount of time left, or -1 if ran out of songs
{
  float x = 0, y = 0, z = 0, speedX = 0, speedY = 0, speedZ = 0;
  float targX = 0, targY = 0, targZ = 0;
  float maxSpeed = 2.0;
  float timeLeft = 0.0;
  int timeLeftMinutes = 0, timeLeftSeconds = 0;
  string timeStr;
  char timeCStr[20];
  ogg.getPosition(x, y, z);
  ogg.getSpeed(speedX, speedY, speedZ);
  ogg.getTarget(targX, targY, targZ);

  x += speedX * timeSecs / SECS_PER_FRAME;
  y += speedY * timeSecs / SECS_PER_FRAME;
  z += speedZ * timeSecs / SECS_PER_FRAME;

  if (ogg.getMoving()) {
    float accel = 0.01 * timeSecs / SECS_PER_FRAME;
    if (z > targZ) speedZ -= accel;
    if (z < targZ) speedZ += accel;
    if (x > targX) speedX -= accel;
    if (x < targX) speedX += accel;
    if (speedX > maxSpeed) speedX = maxSpeed;
    if (speedX < -maxSpeed) speedX = -maxSpeed;
    if (speedZ > maxSpeed) speedZ = maxSpeed;
    if (speedZ < -maxSpeed) speedZ = -maxSpeed;

    // friction
    if (ogg.getFriction()) {
      toTarget(speedX, 0.0, 0.005 * timeSecs / SECS_PER_FRAME);
      toTarget(speedY, 0.0, 0.005 * timeSecs / SECS_PER_FRAME);
      toTarget(speedZ, 0.0, 0.005 * timeSecs / SECS_PER_FRAME);
    }

    // stop
    if (speedX > -0.1 && speedX < 0.1
        && speedY > -0.1 && speedY < 0.1
        && speedZ > -0.1 && speedZ < 0.1
        && x > targX - 10 && x < targX + 10 && y > targY - 10 && y < targY + 10
        && z > targZ - 10 && z < targZ + 10)
    {
      ogg.setMoving(false);
      speedX = 0, speedY = 0, speedZ = 0;
    }
  } // end moving

  // home in
  if (ogg.getHoming()) {
    toTarget(x, 0, 0.1 * timeSecs / SECS_PER_FRAME);
    toTarget(y, 0, 0.1 * timeSecs / SECS_PER_FRAME);
    toTarget(z, -20, 0.1 * timeSecs / SECS_PER_FRAME);
    if (x > -0.1 && x < 0.1 && y > -0.1 && y < 0.1 && z > -20.1 && z < -19.9) {
      x = 0.0, y = 0.0, z = -20.0;
      ogg.setHoming(false);
    }
  }

  ogg.setSpeed(speedX, speedY, speedZ);
  ogg.setPosition(x, y, z);

  // if it's not been opened, or has since finished playing and been released, then leave
  if (!ogg.getSourceInitialised()) {
    if (!playlistNext(ogg)) return -1; // ran out of songs
    if (autoDj && autoDjSource != ogg.getSourceId()) ogg.setPaused(true); // auto pause next song
  }

  // update time left
  timeLeft = ogg.time_total() - ogg.tell();
  timeLeftMinutes = (int) timeLeft / 60;
  timeLeftSeconds = (int) timeLeft - timeLeftMinutes * 60;
  snprintf(timeCStr, 20, "-%02i:%02i", timeLeftMinutes, timeLeftSeconds);
  timeStr = timeCStr;
  playlistMenuTime.changeButton(ogg.getSourceId(), timeStr);

  // play next part of stream
  if (!ogg.getPaused() && !ogg.update())
  {
    //cout << "Ogg finished." << endl;
    ogg.release();
  }

  return (int) timeLeft;
}

void autoDjUpdate(float timeLeft[])
{
  // which source are we dealing with?
  if (autoDjSource == -1) {
    if (ogg[1].getSourceInitialised() && (ogg[0].getX() < -0.1 || ogg[0].getX() > 0.1)) { // 1 takes stage
      autoDjSource = 1;
    }
    if (ogg[0].getSourceInitialised() && (ogg[1].getX() < -0.1 || ogg[1].getX() > 0.1)) { // 0 is taking stage
      autoDjSource = 0;
    }
  }

  if (autoDjSource == 0) {
    if (timeLeft[0] > 40 && (ogg[0].getX() < -0.1 || ogg[0].getX() > 0.1)) { // fly centre
      // release unused song left there from manual DJ
      if (ogg[1].getSourceInitialised() && !ogg[1].getPaused() && (ogg[1].getX() < -950 || ogg[1].getX() > 950)) ogg[1].release();
      ogg[0].setPaused(false);
      ogg[0].setMoving(true);
      ogg[0].setFriction(true);
      ogg[0].setHoming(true);
      //sourceSelected = 0;
      //sourceFlyCentre();
      ogg[0].setTarget(0, 0, -50);
    }
    if (timeLeft[0] > 25 && timeLeft[0] < 40) { // fly 1 in
      ogg[1].setPaused(false);
      ogg[1].setMoving(true);
      ogg[1].setFriction(true);
      ogg[1].setHoming(true);
      //sourceSelected = 1;
      //sourceFlyCentre();
      ogg[1].setTarget(0, 0, -50);
    }
    if (timeLeft[0] > 20 && timeLeft[0] < 25) {
      if (!ogg[0].getMoving() && ogg[0].getX() > -0.1 && ogg[0].getX() < 0.1) { // fly 0 around
        //sourceSelected = 0;
        //sourceMove();
        ogg[0].setMoving(true);
        ogg[0].setSpeed(1.2, 0, 1.5);
        ogg[0].setFriction(false);
        ogg[0].setHoming(false);
      }
    }
    if (timeLeft[0] < 20) { // fly off
      //sourceSelected = 0;
      //sourceFlyLeft();
      ogg[0].setTarget(-1000, 0, -50);
      ogg[0].setMoving(true);
      ogg[0].setFriction(true);
      autoDjSource = 1;
    }
  } // end dj source 0

  if (autoDjSource == 1) {

    if (timeLeft[1] > 40 && (ogg[1].getX() < -0.1 || ogg[1].getX() > 0.1)) { // fly centre
      // release unused song left there from manual DJ
      if (ogg[0].getSourceInitialised() && !ogg[0].getPaused() && (ogg[0].getX() < -950 || ogg[0].getX() > 950)) ogg[0].release();
      ogg[1].setPaused(false);
      ogg[1].setMoving(true);
      ogg[1].setFriction(true);
      ogg[1].setHoming(true);
      //sourceSelected = 1;
      //sourceFlyCentre();
      ogg[1].setTarget(0, 0, -50);
    }
    if (timeLeft[1] > 25 && timeLeft[1] < 40) { // fly 0 in
      ogg[0].setPaused(false);
      ogg[0].setMoving(true);
      ogg[0].setFriction(true);
      ogg[0].setHoming(true);
      //sourceSelected = 0;
      //sourceFlyCentre();
      ogg[0].setTarget(0, 0, -50);
    }
    if (timeLeft[1] > 20 && timeLeft[1] < 25) {
      if (!ogg[1].getMoving() && ogg[1].getX() > -0.1 && ogg[1].getX() < 0.1) { // fly 1 around
        ogg[1].setSpeed(-1.2, 0, 1.5);
        ogg[1].setMoving(true);
        ogg[1].setFriction(false);
        ogg[1].setHoming(false);
      }
    }
    if (timeLeft[1] < 20) { // fly off
      //sourceSelected = 1;
      //sourceFlyRight();
      ogg[1].setTarget(1000, 0, -50);
      ogg[1].setMoving(true);
      ogg[1].setFriction(true);
      autoDjSource = 0;
    }
  } // end dj source 1
}

void doBinaural(float timeSecs)
  // Deal with binaural sound
{
  static float lastX = 0, lastZ = 0, lastX0 = 0, lastZ0 = 0, lastX1 = 0, lastZ1 = 0;

  // if they're both paused then play sound
  static bool startedClock = false;
  if (!startedClock) {
    startedClock = true;
    cout << "STARTING CLOCK" << endl;
    gettimeofday(&binaural.timeStart, NULL);
  }

  float x0 = 0, y0 = 0, z0 = 0;
  float x1 = 0, y1 = 0, z1 = 0;
  ogg[0].getPosition(x0, y0, z0);
  ogg[1].getPosition(x1, y1, z1);

  if (lastX != binaural.x || lastZ != binaural.z
    || lastX0 != x0 || lastZ0 != z0
    || lastX1 != x1 || lastZ1 != z1)
  {
    // one of the sound sources has moved, or the binaural position has moved.

    // ok, we know where it is. So track starts playing at time t, and each
    // source then plays at time t+dist/speedOfSound.  We simulate this by
    // storing the time the track supposedly started playing as
    // binaural.timeStart, and from this we can work out how far through it is
    // (playingTime = current time - timeStart).  dist/speedOfSound is the delay,
    // so we play the track at point playingTime - delay

    // speed of sound at sea level = 340.29 m/s
    double speedOfSound = 340.29;

    double distX = binaural.x - x0, distZ = binaural.z - z0;
    double dist0 = sqrt(distX * distX + distZ * distZ);
    distX = binaural.x - x1, distZ = binaural.z - z1;
    double dist1 = sqrt(distX * distX + distZ * distZ);

    double t0 = dist0/speedOfSound; // delay before sound reaches virtual ears
    double t1 = dist1/speedOfSound;

    // time since started playing
    struct timeval timeSince;
    gettimeofday(&timeSince, NULL);
    timeSince.tv_sec = timeSince.tv_sec - binaural.timeStart.tv_sec;
    timeSince.tv_usec = timeSince.tv_usec - binaural.timeStart.tv_usec;

    if (timeSince.tv_usec < 0) {
      timeSince.tv_sec--;
      timeSince.tv_usec += 1000000;
    }

    // get the time as a 'double' number of seconds
    double playingTime = timeSince.tv_sec + timeSince.tv_usec / 1000000.0;

    if (playingTime - t0 > 0) ogg[0].seek((double) (playingTime - t0));
    //else ogg[0].seek(0);
    if (playingTime - t1 > 0) ogg[1].seek((double) (playingTime - t1));
    //else ogg[1].seek(0);

    static bool started = false;
    if (!started && playingTime - t0 > 0 && playingTime - t1 > 0) {
      started = true;
      ogg[0].seek(0); // temporary TODO remove
      ogg[1].seek(0); // temporary TODO remove

      ogg[0].playback_binaural();
      ogg[1].playback_binaural();
      ogg[0].setPaused(false);
      ogg[1].setPaused(false);
      cout << "Started!" << endl;
    }



    cout << "dist0: " << dist0 << ", dist1: " << dist1 << endl;
    cout << "t0: " << t0 << ", t1: " << t1 << endl;
    cout << "seek0: " << (double) (playingTime - t0) << ", seek1: " << (double) (playingTime - t1) << endl;
    cout << "tell0: " << (double) ogg[0].tell() << ", tell1: " << (double) ogg[1].tell() << endl;
    double phaseDiff = (playingTime - t1) - (playingTime - t0);
    cout << "phase diff: " << (double) phaseDiff << endl;

    if (playingTime - t1 > 0 || playingTime - t0 > 0) {
      lastX = binaural.x, lastZ = binaural.z;
      lastX0 = x0, lastZ0 = z0;
      lastX1 = x1, lastZ1 = z1;
    }

  }

  phaseDiff = ogg[1].tell() - ogg[0].tell();
}

void idle()
  // Called each frame.
  // Update music.
{
  // time in seconds since last frame
  float timeSecs = getTimeSinceLastFrame();
  float timeLeft[SOURCE_MAX];

  for (int i = 0; i < SOURCE_MAX; i++)
    timeLeft[i] = sourceProcess(timeSecs, ogg[i]);

  if (autoDj) autoDjUpdate(timeLeft);

  if (binaural.enabled) doBinaural(timeSecs);

  // selection arrow
  arrowRot += 1.0 * timeSecs / SECS_PER_FRAME;
  if (arrowRot > 360) arrowRot -= 360;

  // move viewpoint
  if (mouseLookEnabled) {
    cam.angleX += mouseMoveY * mouseSensitivity;
    cam.angleY += mouseMoveX * mouseSensitivity;
    mouseMoveX = 0;
    mouseMoveY = 0;
  }
  if (pressUp) {
    cam.z += (-cam.speed * timeSecs / SECS_PER_FRAME) * cos(degToRad(cam.angleY));
    cam.y += (-cam.speed * timeSecs / SECS_PER_FRAME) * sin(degToRad(cam.angleX));
    cam.x += (cam.speed * timeSecs / SECS_PER_FRAME) * sin(degToRad(cam.angleY));
    //cout << "x: " << cam.x << ", y: " << cam.y << ", z: " << cam.z << ", angleX: " << cam.angleX << ", angleY: " << cam.angleY << endl;
    binaural.z -= 0.1;
  }
  if (pressDown) {
    cam.z -= (-cam.speed * timeSecs / SECS_PER_FRAME) * cos(degToRad(cam.angleY));
    cam.y -= (-cam.speed * timeSecs / SECS_PER_FRAME) * sin(degToRad(cam.angleX));
    cam.x -= (cam.speed * timeSecs / SECS_PER_FRAME) * sin(degToRad(cam.angleY));
    binaural.z += 0.1;
  }
  if (pressRight) {
    cam.z += (cam.speed * timeSecs / SECS_PER_FRAME) * sin(degToRad(cam.angleY));
    // this would be relative to angleZ
    //cam.y += (-cam.speed * timeSecs / SECS_PER_FRAME) * sin(degToRad(cam.angleX));
    cam.x += (cam.speed * timeSecs / SECS_PER_FRAME) * cos(degToRad(cam.angleY));
    binaural.x += 0.1;
  }
  if (pressLeft) {
    cam.z -= (cam.speed * timeSecs / SECS_PER_FRAME) * sin(degToRad(cam.angleY));
    //cam.y -= (-cam.speed * timeSecs / SECS_PER_FRAME) * sin(degToRad(cam.angleX));
    cam.x -= (cam.speed * timeSecs / SECS_PER_FRAME) * cos(degToRad(cam.angleY));
    binaural.x -= 0.1;
  }

  // capture
  /*if (captureAudio) {
    if (!pressCapture) {
      captureAudio = false;
      soundMan.captureStop();
    }else{
      if (!soundMan.capture()) { // data unit full
        char* du = new char[FLAG_AUDIO_SIZE];
        int capturedAmount = soundMan.getCaptureDataUnit(du);
        if (capturedAmount > 0) {
          client.sendDataUdp(du, FLAG_AUDIO_SIZE);
        }
        delete [] du;
      }
    }
  }
  if (pressCapture && !captureAudio) {
    soundMan.captureStart();
  }*/

  // autoscroll
  if (playlistDrag > -1) {
    int scrollY = 0;
    if (mouseY < playlistMenu.getY() + 20) {
      scrollY = playlistMenu.scroll((mouseY - (int) playlistMenu.getY() - 20) / 2);
      playlistMenuOptions.scrollTo(scrollY);
      playlistMenuTime.scrollTo(scrollY);
    }
    if (mouseY > playlistMenu.getY() + playlistMenu.getHeight() - 20) {
      scrollY = playlistMenu.scroll((mouseY - (int) playlistMenu.getY() - (int) playlistMenu.getHeight() + 20) / 2);
      playlistMenuOptions.scrollTo(scrollY);
      playlistMenuTime.scrollTo(scrollY);
    }
  }

  // expire messages
  if (message[0].active) {
    struct timeval timeDiff;
    timeDiff.tv_sec = timeCurrent.tv_sec - message[0].time.tv_sec;
    timeDiff.tv_usec = timeCurrent.tv_usec - message[0].time.tv_usec;
    if (timeDiff.tv_usec < 0) {
      timeDiff.tv_sec--;
      timeDiff.tv_usec += 1000000;
    }
    // timeDiff should be small enough to handle now
    float duration = timeDiff.tv_sec + timeDiff.tv_usec / 1000000.0;
    if (duration > messageDuration) {
      // shift down
      for (int j = 0; j < MESSAGE_MAX - 1; j++) message[j] = message[j+1];
      messageNum--;
      message[MESSAGE_MAX - 1].active = false;
    }
  }

  //usleep(100); // take it easy

  glutPostRedisplay();
}





//
// draw
//
void drawObject(float x, float y, float z, float angleX, float angleY, float angleZ, float sizeX, float sizeY, float sizeZ, float red, float green, float blue)
{
  glPushMatrix();
  glTranslatef(x, y, z);
  glRotatef(angleX, 1, 0, 0);
  glRotatef(angleY+180, 0, 1, 0); // draw facing -z
  glRotatef(angleZ, 0, 0, 1);

  glColor4f(red, green, blue, 1.0);

  sizeX /= 2.0, sizeY /= 2.0, sizeZ /= 2.0;

  glBegin(GL_TRIANGLES);
  glNormal3f(0.0, 1.0, 0.0);
  glVertex3f(-sizeX, sizeY, sizeZ); // top
  glVertex3f(sizeX, sizeY, sizeZ);
  glVertex3f(0, sizeY, -sizeZ);

  glNormal3f(0.0, -1.0, 0.0);
  glVertex3f(-sizeX, -sizeY, sizeZ); // bottom
  glVertex3f(0, -sizeY, -sizeZ);
  glVertex3f(sizeX, -sizeY, sizeZ);
  glEnd();

  glBegin(GL_QUADS);
  glNormal3f(-0.7, 0.0, -0.7); // left
  glVertex3f(0, -sizeY, -sizeZ);
  glVertex3f(-sizeX, -sizeY, sizeZ);
  glVertex3f(-sizeX, sizeY, sizeZ);
  glVertex3f(0, sizeY, -sizeZ);

  glNormal3f(0.7, 0.0, -0.7); // right
  glVertex3f(sizeX, -sizeY, sizeZ);
  glVertex3f(0, -sizeY, -sizeZ);
  glVertex3f(0, sizeY, -sizeZ);
  glVertex3f(sizeX, sizeY, sizeZ);

  glNormal3f(0.0, 0.0, 1.0); // Back
  glVertex3f(-sizeX, -sizeY, sizeZ);
  glVertex3f(sizeX, -sizeY, sizeZ);
  glVertex3f(sizeX, sizeY, sizeZ);
  glVertex3f(-sizeX, sizeY, sizeZ);
  glEnd();

  glPopMatrix();
}

void drawSource(ogg_stream &ogg, float red, float green, float blue)
{
  float x = 0, y = 0, z = 0;
  //float angleX = 0;
  float angleY = 0;
  //float angleZ = 0;
  ogg.getPosition(x, y, z);

  if (ogg.getSpeedX() > 0) {
    if (ogg.getSpeedZ() > 0) {
      angleY = radToDeg(atan(ogg.getSpeedX()/ogg.getSpeedZ()));
    }else if (ogg.getSpeedZ() < 0) {
      angleY = 90+radToDeg(atan(-ogg.getSpeedZ()/ogg.getSpeedX()));
    }else angleY = 90;
  }else if (ogg.getSpeedX() < 0) {
    if (ogg.getSpeedZ() < 0) {
      angleY = 180+radToDeg(atan(ogg.getSpeedX()/ogg.getSpeedZ()));
    }else if (ogg.getSpeedZ() > 0) {
      angleY = 270+radToDeg(atan(ogg.getSpeedZ()/-ogg.getSpeedX()));
    }else angleY = -90;
  }else{
    if (ogg.getSpeedZ() < 0) angleY = 180;
    else angleY = 0;
  }


  //cout << "angleY: " << angleY << endl;
  //angleX = radToDeg(atan(ogg.getSpeedY()/ogg.getSpeedZ()));

  drawObject(x, y, z, 0, angleY, 0, 4, 4, 8, red, green, blue);
}

void drawArrow(ogg_stream &ogg)
  // draw arrow above ogg source
{
  float x = 0, y = 0, z = 0;
  ogg.getPosition(x, y, z);

  glPushMatrix();

  glDisable(GL_CULL_FACE);

  glTranslatef(x, y + 3, z);
  glRotatef(arrowRot, 0, 1, 0);

  glColor4f(0.8, 0.2, 0.2, 1.0);
  glNormal3f(0.0, 0.0, 1.0);
  glBegin(GL_TRIANGLES);
  glVertex3f(0, 0, 0);
  glVertex3f(4, 5, 0);
  glVertex3f(-4, 5, 0);
  glEnd();

  glBegin(GL_QUADS);
  glVertex3f(-2, 4.9, 0);
  glVertex3f(2, 4.9, 0);
  glVertex3f(2, 10, 0);
  glVertex3f(-2, 10, 0);
  glEnd();

  glEnable(GL_CULL_FACE);

  glPopMatrix();
}

void draw3D(int renderMode)
{
  // using right handed coord system
  // (i.e. palm facing you, thumb is positive X, first finger is Y, middle finger is Z)
  if (renderMode == GL_RENDER) {
    glColor4f(0.8, 0.8, 0.8, 1.0);

    // draw ground
    glPushMatrix();
    glTranslatef(0, -50, 0);
    glNormal3f(0.0, 1.0, 0.0);
    glBegin(GL_QUADS);
    for (int x = -50; x < 50; x += 5) {
      for (int z = -50; z < 50; z += 5) {
        glVertex3f(x, 0, z+5);
        glVertex3f(x+5, 0, z+5);
        glVertex3f(x+5, 0, z);
        glVertex3f(x, 0, z);
      }
    }
    glEnd();
    glPopMatrix();

    // draw listener
    drawObject(0, 0, 0, 0, 180, 0, 3, 3, 6, 0.3, 0.8, 0.3);
  }

  // draw source
  float red = 0.7, green = 0.4, blue = 0.2;
  for (int i = 0; i < SOURCE_MAX; i++) {
    glLoadName(i);
    if (i == 1) { red = 0.3, green = 0.1, blue = 0.8; }
    drawSource(ogg[i], red, green, blue);
    if (renderMode == GL_RENDER && sourceSelected == i) drawArrow(ogg[i]);
  }

  if (binaural.enabled) {
    // draw binaural
    glLoadName(SOURCE_MAX); // use next number for name
    drawObject(binaural.x, binaural.y, binaural.z, 0, 0, 0, 3, 3, 6, 0.2, 0.2, 0.2);
  }

  // draw cube (to test lighting)
  /*glBegin(GL_QUADS);
  glNormal3f(0.0, 1.0, 0.0);
  glVertex3f(-10, 10, 10); // top
  glVertex3f(10, 10, 10);
  glVertex3f(10, 10, -10);
  glVertex3f(-10, 10, -10);

  glNormal3f(0.0, 0.0, 1.0);
  glVertex3f(-10, -10, 10); // front
  glVertex3f(10, -10, 10);
  glVertex3f(10, 10, 10);
  glVertex3f(-10, 10, 10);

  glNormal3f(-1.0, 0.0, 0.0);
  glVertex3f(-10, -10, -10); // left
  glVertex3f(-10, -10, 10);
  glVertex3f(-10, 10, 10);
  glVertex3f(-10, 10, -10);

  glNormal3f(1.0, 0.0, 0.0);
  glVertex3f(10, -10, 10); // right
  glVertex3f(10, -10, -10);
  glVertex3f(10, 10, -10);
  glVertex3f(10, 10, 10);

  glNormal3f(0.0, 0.0, -1.0); // back
  glVertex3f(10, -10, -10);
  glVertex3f(-10, -10, -10);
  glVertex3f(-10, 10, -10);
  glVertex3f(10, 10, -10);

  glNormal3f(0.0, -1.0, 0.0);
  glVertex3f(-10, -10, -10); // bottom
  glVertex3f(10, -10, -10);
  glVertex3f(10, -10, 10);
  glVertex3f(-10, -10, 10);
  glEnd();*/
}

void drawText(string str, float x, float y)
{
  glRasterPos2f(x, y);
  for(unsigned int s = 0; s < str.length(); s++)
  {
    // this moves raster position on by the right amount
    //glutBitmapCharacter(GLUT_BITMAP_9_BY_15, str[s]);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, str[s]);
    //x += 10.5; // useful to know char width
  }
}

int drawTextWrap(string str, float x, float y, float right)
  // wraps and returns number of lines
{
  int lines = 1;

  glRasterPos2f(x, y);
  float pos[4]; // x, y, z, w
  for(unsigned int s = 0; s < str.length(); s++)
  {
    // this moves raster position on by the right amount
    //glutBitmapCharacter(GLUT_BITMAP_9_BY_15, str[s]);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, str[s]);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, pos);
    if (pos[0] > right) {
      y += 15.0;
      lines++;
      glRasterPos2f(x, y);
    }
  }

  return lines;
}

void drawInfoBox()
{
  glColor4f(0.2, 0.2, 0.2, 0.5);
  glBegin(GL_QUADS);
  glVertex2f(260, 50);
  glVertex2f(260, 285);
  glVertex2f(windowWidth - 275, 285);
  glVertex2f(windowWidth - 275, 50);
  glEnd();

  glColor4f(1.0, 1.0, 1.0, 1.0);
  drawText("Now playing:", 265, 65);
  drawText("Next:", 265, 185);
  // find nearest source to the centre, that's the current one.
  int s = getSourceNearestCentre();
  int s2 = (s == 0) ? 1 : 0;
  for (int i = 0; i < playlistNum; i++) {
    if (playlist[i].source == s && ogg[s].getSourceInitialised()) {
      drawText(ogg[s].getTitle(), 275, 80);
      drawText(ogg[s].getArtist(), 275, 95);
      drawText(ogg[s].getDate(), 275, 110);
      drawText(ogg[s].getAlbum(), 275, 125);
      drawText("Track " + ogg[s].getTracknumber(), 275, 140);
      drawText("(" + ogg[s].getGenre() + ")", 275, 155);
    }
    if (playlist[i].source == s2 && ogg[s2].getPaused() && ogg[s2].getSourceInitialised()) {
      drawText(ogg[s2].getTitle(), 275, 200);
      drawText(ogg[s2].getArtist(), 275, 215);
      drawText(ogg[s2].getDate(), 275, 230);
      drawText(ogg[s2].getAlbum(), 275, 245);
      drawText("Track " + ogg[s2].getTracknumber(), 275, 260);
      drawText("(" + ogg[s2].getGenre() + ")", 275, 275);
    }
  }

}

void drawMessages()
{
  float messageBottom = playlistMenu.getY() + playlistMenu.getHeight();
  float messageX = 160.0, messageY = messageBottom - messageNum * 15 - messageWrapped * 15 + 7;
  float messageRight = windowWidth - 275; //messageX + 365.0;
  if (messageNum > 0) {
    glColor4f(0.2, 0.2, 0.2, 0.5);
    glBegin(GL_QUADS);
    glVertex2f(messageX, messageBottom);
    glVertex2f(messageRight, messageBottom);
    glVertex2f(messageRight, messageY - 15);
    glVertex2f(messageX, messageY - 15);
    glEnd();
  }

  glColor4f(1.0, 1.0, 1.0, 1.0);
  int lines = 0;
  messageWrapped = 0; // count up number of wrapped lines here

  for (int i = 0; i < messageNum; i++) {
    if (message[i].active) { // which it should be
      lines = drawTextWrap(message[i].text, messageX + 5, messageY, messageRight - 5);
      messageY += 15.0 * lines;
      messageWrapped += lines - 1;
    }
  }
}

void draw2D()
{
  playlistMenu.draw(0);
  playlistMenuOptions.draw(0);
  playlistMenuTime.draw(0);

  if (playlistDrag > -1) {
    // draw text of song being dragged
    glColor4f(1.0, 1.0, 1.0, 1.0);
    drawText(playlist[playlistDrag].title, mouseX, mouseY);
  }

  quickDjMenu.draw(autoDj * BUTTON_AUTO_DJ);

  // info box
  drawInfoBox();

  int params = 0;
  for (int i = 0; i < SOURCE_MAX; i++) {
    if (ogg[i].getMoving()) params += BUTTON_MOVE;
    if (ogg[i].getHoming()) params += BUTTON_HOMING;
    if (ogg[i].getFriction()) params += BUTTON_FRICTION;
    float targX = 0, targY = 0, targZ = 0;
    ogg[i].getTarget(targX, targY, targZ);
    if (targX == 0) params += BUTTON_FLY_CENTRE;
    if (targX < -999) params += BUTTON_FLY_LEFT;
    if (targX > 999) params += BUTTON_FLY_RIGHT;
    if (ogg[i].getPaused()) params += BUTTON_PAUSE;
    sourceMenu[i].draw(params);
    params = 0;
  }

  // draw coords
  glColor4f(1.0, 1.0, 1.0, 1.0);
  float x = 0, y = 0, z = 0;
  for (int i = 0; i < SOURCE_MAX; i++) {
    ogg[i].getPosition(x, y, z);
    char s[20];
    snprintf(s, 20, "X: %.0f Z: %.0f", x, z);
    string str = s;
    drawText(str, sourceMenu[i].getX() + 5, sourceMenu[i].getY() + sourceMenu[i].getHeight() - 5);
  }

  // draw phase diff
  //double phaseDiff = ogg[1].tell() - ogg[0].tell();
  char phaseText[20];
  snprintf(phaseText, 20, "phase: %f", phaseDiff);
  string str = phaseText;
  drawText(str, sourceMenu[1].getX() + 5, sourceMenu[1].getY() + sourceMenu[1].getHeight() + 5);
  //static double lastPhaseDiff = 0;
  if (lastPhaseDiff != phaseDiff) {
    //cout << "phase diff: " << (double) phaseDiff << endl;
    lastPhaseDiff = phaseDiff;
  }

  drawMessages();

  // coloured bars above source menus
  glColor4f(0.7, 0.4, 0.2, 1.0);
  glBegin(GL_QUADS);
  glVertex2f(sourceMenu[0].getX(), sourceMenu[0].getY());
  glVertex2f(sourceMenu[0].getX() + sourceMenu[0].getWidth(), sourceMenu[0].getY());
  glVertex2f(sourceMenu[0].getX() + sourceMenu[0].getWidth(), sourceMenu[0].getY() - 5);
  glVertex2f(sourceMenu[0].getX(), sourceMenu[0].getY() - 5);
  glEnd();
  glColor4f(0.3, 0.1, 0.8, 1.0);
  glBegin(GL_QUADS);
  glVertex2f(sourceMenu[1].getX(), sourceMenu[1].getY());
  glVertex2f(sourceMenu[1].getX() + sourceMenu[1].getWidth(), sourceMenu[1].getY());
  glVertex2f(sourceMenu[1].getX() + sourceMenu[1].getWidth(), sourceMenu[1].getY() - 5);
  glVertex2f(sourceMenu[1].getX(), sourceMenu[1].getY() - 5);
  glEnd();

  // bar showing selected source
  if (sourceSelected > -1) {
    glColor4f(1.0, 1.0, 0.0, 1.0);
    glBegin(GL_QUADS);
    glVertex2f(sourceMenu[sourceSelected].getX() + sourceMenu[sourceSelected].getWidth(), sourceMenu[sourceSelected].getY());
    glVertex2f(sourceMenu[sourceSelected].getX() + sourceMenu[sourceSelected].getWidth(), sourceMenu[sourceSelected].getY() + sourceMenu[sourceSelected].getHeight());
    glVertex2f(sourceMenu[sourceSelected].getX() + sourceMenu[sourceSelected].getWidth() + 5, sourceMenu[sourceSelected].getY() + sourceMenu[sourceSelected].getHeight());
    glVertex2f(sourceMenu[sourceSelected].getX() + sourceMenu[sourceSelected].getWidth() + 5, sourceMenu[sourceSelected].getY());
    glEnd();
  }
}

void frustum()
  // Set up viewing frustum
  // Call in GL_PROJECTION matrix mode
{
  //        left                 right               bottom      top        near       far
  glFrustum(-rightFrustumBorder, rightFrustumBorder, -nearPlane, nearPlane, nearPlane, farPlane);
}

void display()
{
  // get render mode
  int renderMode;
  glGetIntegerv(GL_RENDER_MODE, &renderMode);

  // only setup projection matrix if rendering (otherwise projection matrix will be set up for selection!)
  if (renderMode == GL_RENDER) {
    glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    frustum();
    glMatrixMode (GL_MODELVIEW);
  }
  glLoadIdentity();

  glRotatef(cam.angleX, 1.0, 0.0, 0.0);
  glRotatef(cam.angleY, 0.0, 1.0, 0.0);

  glTranslatef(-cam.x, -cam.y, -cam.z);

  glLightfv (GL_LIGHT0, GL_POSITION, lightCoords0); // two lights to give each surface a different brightness
  glLightfv (GL_LIGHT1, GL_POSITION, lightCoords1);
  glLightfv (GL_LIGHT2, GL_POSITION, lightCoords2); // positional and attenuated light for atmosphere

  draw3D(renderMode);

  if (renderMode == GL_RENDER) {
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    // 0,0 in top left
    //       left  right        bottom        top
    gluOrtho2D( 0, windowWidth, windowHeight, 0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    draw2D();

    glEnable(GL_DEPTH_TEST);
  }

  glutSwapBuffers();
}

void reshape (int w, int h)
{
  cout << "Viewport: " << w << ", " << h << endl;
  // this function is called once when window is initialised
  glViewport (0, 0, (GLsizei) w, (GLsizei) h);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  frustum();

  glMatrixMode (GL_MODELVIEW);
}



//
// input
// 
void keyboard(unsigned char key, int x, int y)
{
  static double time = 0;
  char str[50];

  switch (key) {
    case 27: // ESC
      exit(0);
      break;
    case '0':
      sourceSelected = 0;
      break;
    case '1':
      sourceSelected = 1;
      break;
    case 'a':
      autoDj = !autoDj;
      if (autoDj) snprintf(str, 50, "Auto DJ on");
      else {
        snprintf(str, 50, "Auto DJ off");
        autoDjSource = -1;
      }
      messageAdd(str);
      break;
    case 'z':
      if (sourceSelected > -1) {
        sourceRewindToStart();
        snprintf(str, 50, "Rewinding source %i to start", sourceSelected);
        messageAdd(str);
      }
      break;
    case 'x':
      if (sourceSelected > -1) {
        sourceRewind();
        snprintf(str, 50, "Rewinding source %i", sourceSelected);
        messageAdd(str);
      }
      break;
    case 'c':
      if (sourceSelected > -1) {
        sourcePause();
        if (ogg[sourceSelected].getPaused()) snprintf(str, 50, "Paused source %i", sourceSelected);
        else snprintf(str, 50, "Unpaused source %i", sourceSelected);
        messageAdd(str);
      }
      break;
    case 'v':
      if (sourceSelected > -1) {
        sourceFastforward();
        snprintf(str, 50, "Fast forwarding source %i", sourceSelected);
        messageAdd(str);
      }
      break;
    case 'b':
      if (sourceSelected > -1) {
        sourceSkip();
        snprintf(str, 50, "Skipping source %i", sourceSelected);
        messageAdd(str);
      }
      break;
    case 'n':
      sourceNext();
      messageAdd("Playing next source");
      break;
    case 'h':
      messageHelp();
      break;
    case 's':
      playlistShuffle();
      break;
    case 'u':
      playlistUncrossAll();
      break;
    case 'r': // store point
      if (sourceSelected > -1) time = ogg[sourceSelected].tell();
      break;
    case 't': // rewind to point
      if (sourceSelected > -1) ogg[sourceSelected].seek(time);
      break;
    case 'g':
      sourceCentre();
      break;
    case 'l':
      //playlistReload();
      ogg[1].setPitch(1.004, true); // TODO quick hack
      ogg[0].setPitch(1.0, true); // TODO quick hack
      break;
    case 'm':
      //sourceMove();
      ogg[0].setPitch(1.004, true); // TODO quick hack
      ogg[1].setPitch(1.0, true); // TODO quick hack
      break;
    case 'f':
      sourceFriction();
      break;
    case ';':
      mouseLookEnabled = !mouseLookEnabled;
      if (mouseLookEnabled) glutWarpPointer(windowCentreX, windowCentreY);
      break;
    case 'o':
      sourceHome();
      break;
    case 'y':
      playlistTidy();
      break;
  }
}

void keyboardUp(unsigned char key, int x, int y)
{
}

void specialKeyPress(int key, int x, int y)
{
  //specialKey = glutGetModifiers();

  switch (key) {
    case GLUT_KEY_UP:
      pressUp = true;
      break;
    case GLUT_KEY_RIGHT:
      pressRight = true;
      break;
    case GLUT_KEY_LEFT:
      pressLeft = true;
      break;
    case GLUT_KEY_DOWN:
      pressDown = true;
      break;
    //case GLUT_PAGE_DOWN:
      //pressCapture = true;
      //break;
  }
}

void specialKeyUp(int key, int x, int y)
{
  switch (key) {
    case GLUT_KEY_UP:
      pressUp = false;
      break;
    case GLUT_KEY_RIGHT:
      pressRight = false;
      break;
    case GLUT_KEY_LEFT:
      pressLeft = false;
      break;
    case GLUT_KEY_DOWN:
      pressDown = false;
      break;
  }
}

void mouseLook()
{
  if (mouseX != windowCentreX || mouseY != windowCentreY) {
    // grab distance mouse has moved
    mouseMoveX = mouseX - windowCentreX;
    mouseMoveY = mouseY - windowCentreY;
    if (mouseInvert) mouseMoveY = -mouseMoveY;

    glutWarpPointer(windowCentreX, windowCentreY);
  }
}

void mousePassiveMove(int x, int y)
{
  mouseX = x, mouseY = y;

  if (mouseLookEnabled) mouseLook();

  quickDjMenu.highlightButtons(x, y);
  playlistMenu.highlightButtons(x, y);
  playlistMenuOptions.highlightButtons(x, y);
  int overButton = playlistMenuOptions.checkOverButtonNum(x, y);
  if (overButton > -1) sourceSelected = overButton;
  for (int i = 0; i < SOURCE_MAX; i++) {
    if (sourceMenu[i].checkOver(x, y)) sourceSelected = i;
    sourceMenu[i].highlightButtons(x, y);
  }
}

void mouseActiveMove(int x, int y)
  // button is pressed
{
  mouseX = x, mouseY = y;

  if (playlistDrag > -1 && playlistMenu.checkOver(x, y)) {
    // displace menu
    int over = (int) (y - playlistMenu.getY() - 7 + playlistMenu.getScrollY()) / 15;
    if (over > -1) {
      for (int i = 0; i < over && i < playlistNum; i++) { // move the preceding to normal
        playlistMenu.moveButton(i, 5, i * 15 + 5);
        if (playlist[i].source > -1) {
          playlistMenuOptions.moveButton(playlist[i].source, 5, i * 15 + 5);
          playlistMenuTime.moveButton(playlist[i].source, 5, i * 15 + 5);
        }
      }
      for (int i = over; i < playlistNum; i++) {
        playlistMenu.moveButton(i, 5, i * 15 + 20);
        if (playlist[i].source > -1) {
          playlistMenuOptions.moveButton(playlist[i].source, 5, i * 15 + 20);
          playlistMenuTime.moveButton(playlist[i].source, 5, i * 15 + 20);
        }
      }
    }
  }

  if (scrolling) {
    int scrollY = playlistMenu.scroll(y - mouseStoreY);
    playlistMenuOptions.scrollTo(scrollY);
    playlistMenuTime.scrollTo(scrollY);
    mouseStoreY = y;
  }

  if (pressRightButton) {
    // check its warped to centre
    //if (x > mouseStoreX - 20 && x < mouseStoreX + 20
    //    && y > mouseStoreY - 20 && y < mouseStoreY + 20
    //    && (mouseStoreX < windowCentreX - 20 || mouseStoreX > windowCentreX + 20
    //      || mouseStoreY < windowCentreY - 20 || mouseStoreY > windowCentreY + 20)
    if (!warped && (x < mouseStoreX - 20 || x > mouseStoreX + 20
          || y < mouseStoreY - 20 || y > mouseStoreY + 20
          || mouseStoreX < windowCentreX + 20 && mouseStoreX > windowCentreX - 20
          && mouseStoreY < windowCentreY + 20 && mouseStoreY > windowCentreY - 20))
    {
      warped = true;
    }
    if (warped) {

      mouseLook();

      if (sourceSelected > -1) {
        float x = 0, y = 0, z = 0;

        ogg[sourceSelected].getPosition(x, y, z);

        x += 0.5 * mouseMoveX * mouseSensitivity * cos(degToRad(cam.angleY))
          - 0.5 * mouseMoveY * mouseSensitivity * sin(degToRad(cam.angleY));
        z += 0.5 * mouseMoveX * mouseSensitivity * sin(degToRad(cam.angleY))
          + 0.5 * mouseMoveY * mouseSensitivity * cos(degToRad(cam.angleY));

        ogg[sourceSelected].setPosition(x, y, z);
      }
    }
  }

}

// Deals with block selection by looking up hit records and finding
// the nearest hit. Code inspired from OpenGL: A Primer, E. Angel (2004)
//   hits - the number of hits that took place
//   buffer - an array storing the hit records
void selectionProcess(GLint hits, GLuint buffer[])
{
  GLuint *ptr;
  GLuint nearestDepth, numIds;
  GLint nearestId = -1;

  ptr = buffer; 

  // each time a name is changed or render mode is changed,
  // a hit record is recorded in buffer
  // a record consists of number of names in stack,
  // min depth, max depth, and then the names
  for (int i = 0; i < hits; i++, ptr+=(3+numIds)) {	// for each hit
    numIds = *ptr;

    if( i == 0) { // First hit
      nearestDepth = *(ptr+1);
      nearestId = *(ptr+3);
    }
    else if( *(ptr+1) < nearestDepth ) { // Nearer
      nearestDepth = *(ptr+1);
      nearestId = *(ptr+3);
    }
  }

  if( nearestId > -1) {
    sourceSelected = nearestId;
  }else{
    sourceSelected = -1;
  }
}

#define SELECT_SIZE 20

void mousePress(int button, int state, int x, int y)
{
  // The select buffer is used for storing hit records
  GLuint selectBuf[SELECT_SIZE];
  GLint hits;
  GLint viewport[4];

  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      // if in playlist menu
      if (playlistMenu.checkOver(x, y)) {
        // if scrolling
        if (playlistMenu.checkOverScrollbar(x, y)) {
          scrolling = true;
          mouseStoreY = y;
        }else
          playlistDrag = playlistMenu.checkOverButtonNum(x, y);
      }else if (quickDjMenu.checkOver(x, y)) {
        // if in quick DJ menu
        int clicked = quickDjMenu.checkOverButton(x, y);
        switch (clicked) {
          case BUTTON_AUTO_DJ:
            autoDj = !autoDj;
            if (!autoDj) autoDjSource = -1;
            break;
          case BUTTON_NEXT_SOURCE:
            sourceNext();
            break;
          case BUTTON_SHUFFLE:
            playlistShuffle();
            break;
          case BUTTON_UNCROSS_ALL:
            playlistUncrossAll();
            break;
          case BUTTON_TIDY:
            playlistTidy();
            break;
        }
      }else{
        // if in source menu (or playlist options menu)
        if (sourceSelected > -1 && (sourceMenu[sourceSelected].checkOver(x, y) || playlistMenuOptions.checkOver(x, y))) {
          int clicked = -1;
          if (sourceMenu[sourceSelected].checkOver(x, y)) clicked = sourceMenu[sourceSelected].checkOverButton(x, y);
          else clicked = playlistMenuOptions.checkOverButton(x, y);
          switch (clicked) {
            case BUTTON_CENTRE:
              sourceCentre();
              break;
            case BUTTON_REWIND_START:
              sourceRewindToStart();
              break;
            case BUTTON_REWIND:
              sourceRewind();
              break;
            case BUTTON_PAUSE:
              sourcePause();
              break;
            case BUTTON_FORWARD:
              sourceFastforward();
              break;
            case BUTTON_SKIP:
              sourceSkip();
              playlistMenuOptions.highlightButtons(-1, -1); // hack to unhighlight button once skipped
              break;
            case BUTTON_MOVE:
              sourceMove();
              break;
            case BUTTON_HOMING:
              sourceHome();
              break;
            case BUTTON_FRICTION:
              sourceFriction();
              break;
            case BUTTON_FLY_CENTRE:
              sourceFlyCentre();
              break;
            case BUTTON_FLY_LEFT:
              sourceFlyLeft();
              break;
            case BUTTON_FLY_RIGHT:
              sourceFlyRight();
              break;
          }
        }else{
          // pick a block
          glGetIntegerv (GL_VIEWPORT, viewport);

          glSelectBuffer (SELECT_SIZE, selectBuf);
          glRenderMode(GL_SELECT);

          glInitNames(); // Empty stack
          glPushName(0); // Put 0 on stack (glLoadName changes top value in stack)

          glMatrixMode (GL_PROJECTION);
          glPushMatrix ();

          // load the identity matrix and start from scratch,
          // because gluPickMatrix needs to be called first before glFrustum
          glLoadIdentity ();

          // create small picking region near cursor location
          // cursor y is measured from top of screen, so viewport[3] - y is screen height minus y
          // which gets it from bottom of screen
          //                 x,                 y,                                width, height, viewport
          gluPickMatrix (x, viewport[3] - y, 4.0, 4.0, viewport);

          frustum();

          // render the scene within this viewport - any blocks rendered count as a hit
          glMatrixMode (GL_MODELVIEW);
          display();

          glMatrixMode (GL_PROJECTION);
          glPopMatrix (); // pop the projection matrix back to what it was
          glMatrixMode (GL_MODELVIEW);

          hits = glRenderMode (GL_RENDER);
          selectionProcess (hits, selectBuf);
        }
      } // end not playlist menu
    }else{ // left button up
      // deal with double click
      struct timeval time;
      gettimeofday(&time, NULL);
      timeSince.tv_sec = time.tv_sec - timePress.tv_sec;
      timeSince.tv_usec = time.tv_usec - timePress.tv_usec;

      if (timeSince.tv_usec < 0) {
        timeSince.tv_sec--;
        timeSince.tv_usec += 1000000;
      }

      timePress.tv_sec = time.tv_sec;
      timePress.tv_usec = time.tv_usec;

      if (timeSince.tv_sec < 1 && timeSince.tv_usec < DOUBLE_CLICK_DELAY) {
        sourceEnqueue(x, y);
      }

      // stop the drag
      scrolling = false;
      if (playlistDrag > -1) {
        if (playlistMenu.checkOver(x, y)) {
          // shift playlist items
          int swap = (int) (y - playlistMenu.getY() - 7 + playlistMenu.getScrollY()) / 15;
          if (swap > -1 && (swap < playlistNum || swap > playlistDrag - 1 && swap - 1 < playlistNum) && playlistDrag < playlistNum && swap != playlistDrag) {
            Playlist temp = playlist[playlistDrag];
            string text = playlistMenu.getButtonText(playlistDrag);
            bool played = playlistMenu.getPlayed(playlistDrag);

            if (swap < playlistDrag) {
              // moving item up, list goes down to make room
              for (int i = playlistDrag; i > swap && i > 0; i--) {
                playlist[i] = playlist[i-1];
                playlistMenu.changeButton(i, playlistMenu.getButtonText(i-1));
                playlistMenu.setPlayed(i, playlistMenu.getPlayed(i-1));
              }
            }else{
              // moving item down, list goes up to make room
              swap--; // (account for gap)
              for (int i = playlistDrag; i < swap && i < playlistNum - 1; i++) {
                playlist[i] = playlist[i+1];
                playlistMenu.changeButton(i, playlistMenu.getButtonText(i+1));
                playlistMenu.setPlayed(i, playlistMenu.getPlayed(i+1));
              }
            }
            playlist[swap] = temp;
            playlistMenu.changeButton(swap, text);
            playlistMenu.setPlayed(swap, played);
          }
        }
        for (int i = 0; i < playlistNum; i++) {
          playlistMenu.moveButton(i, 5, i * 15 + 5);
          if (playlist[i].source > -1) {
            playlistMenuOptions.moveButton(playlist[i].source, 5, i * 15 + 5);
            playlistMenuTime.moveButton(playlist[i].source, 5, i * 15 + 5);
          }
        }
        playlistDrag = -1;
        playlistMenu.highlightButtons(x, y);
      }
    }
  } // end left button

  if (button == GLUT_RIGHT_BUTTON) {
    if (state == GLUT_DOWN) {
      int playlistButton = playlistMenu.checkOverButtonNum(x, y);
      if (playlistButton > -1) {
        if (playlist[playlistButton].source < 0) { // no source attached
          playlistMenu.setPlayed(playlistButton, !playlistMenu.getPlayed(playlistButton));
          if (playlistMenu.getPlayed(playlistButton)) {
            playlist[playlistButton].source = -2;
          }else{
            playlist[playlistButton].source = -1;
          }
        }
      }else{
        pressRightButton = true;
        mouseStoreX = x;
        mouseStoreY = y;
        glutSetCursor(GLUT_CURSOR_NONE);
        glutWarpPointer(windowCentreX, windowCentreY);
        warped = false; // there's a delay before it's warped... truth!
      }
    }else{
      if (pressRightButton) {
        pressRightButton = false;
        glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
        glutWarpPointer(mouseStoreX, mouseStoreY);
      }
    }
  }

  if (button == GLUT_MIDDLE_BUTTON) {
    if (state == GLUT_DOWN) {
      if (playlistMenu.checkOver(x, y)) {
        // display info
        int n = playlistMenu.checkOverButtonNum(x, y);
        if (n > -1 && n < playlistNum) {
          char str[200];
          snprintf(str, 200, "Filename: %s", playlist[n].file.c_str());
          messageAdd(str);
        }
      }
    }
  }
}







//
// initialisation
//
void init()
{
  // graphics
  glClearColor (189.0/255.0, 215.0/255.0, 216.0/255.0, 1.0);

  windowWidth = glutGet(GLUT_WINDOW_WIDTH);
  windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
  cout << "Window width: " << windowWidth << ", height: " << windowHeight << endl;
  windowCentreX = windowWidth/2;
  windowCentreY = windowHeight/2;
  rightFrustumBorder = nearPlane * (float) windowWidth / (float) windowHeight;

  glEnable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_COLOR_MATERIAL);

  glLightfv (GL_LIGHT0, GL_AMBIENT, ambient0);
  glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse0);
  glLightfv (GL_LIGHT0, GL_SPECULAR, specular0);

  glLightfv (GL_LIGHT1, GL_AMBIENT, ambient1);
  glLightfv (GL_LIGHT1, GL_DIFFUSE, diffuse1);
  glLightfv (GL_LIGHT1, GL_SPECULAR, specular1);

  // positional
  glLightfv (GL_LIGHT2, GL_AMBIENT, ambient2);
  glLightfv (GL_LIGHT2, GL_DIFFUSE, diffuse2);
  glLightfv (GL_LIGHT2, GL_SPECULAR, specular2);
  glLightf (GL_LIGHT2, GL_CONSTANT_ATTENUATION, 3);
  //glLightf (GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.1);
  //glLightf (GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.01);

  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHT2);
  glEnable(GL_LIGHTING);

  glEnable (GL_CULL_FACE);

  // timing
  gettimeofday(&timeLast, NULL);
  gettimeofday(&timePress, NULL);

  // centre mouse pointer
  if (mouseLookEnabled) {
    glutWarpPointer(windowCentreX, windowCentreY);
  }

  // camera
  cam.x = 0, cam.y = 59, cam.z = 54;
  cam.angleX = 56, cam.angleY = 0, cam.angleZ = 0;
  cam.speed = 1.0;

  // menus
  float menuY = 50;
  for (int i = 0; i < SOURCE_MAX; i++) {
    sourceMenu[i].set(5, menuY, 150, 205, 1);
    sourceMenu[i].addButton(BUTTON_MOVE, "Move", 5, 5);
    sourceMenu[i].addButton(BUTTON_FRICTION, "Friction", 5, 20);
    sourceMenu[i].addButton(BUTTON_HOMING, "Home", 5, 35);
    sourceMenu[i].addButton(BUTTON_FLY_CENTRE, "Fly centre", 5, 50);
    sourceMenu[i].addButton(BUTTON_FLY_LEFT, "Fly away left", 5, 65);
    sourceMenu[i].addButton(BUTTON_FLY_RIGHT, "Fly away right", 5, 80);
    sourceMenu[i].addButton(BUTTON_CENTRE, "Centre", 5, 95);
    sourceMenu[i].addButton(BUTTON_REWIND_START, "Rewind to start", 5, 110);
    sourceMenu[i].addButton(BUTTON_REWIND, "Rewind 10 secs", 5, 125);
    sourceMenu[i].addButton(BUTTON_PAUSE, "Pause", 5, 140);
    sourceMenu[i].addButton(BUTTON_FORWARD, "Forward 10 secs", 5, 155);
    sourceMenu[i].addButton(BUTTON_SKIP, "Skip", 5, 170);
    menuY += 220;
  }

  quickDjMenu.set(sourceMenu[0].getX() + sourceMenu[0].getWidth() + 5, 50, 100 - sourceMenu[0].getX(), 85, 1);
  quickDjMenu.addButton(BUTTON_AUTO_DJ, "Auto DJ [a]", 5, 5);
  quickDjMenu.addButton(BUTTON_NEXT_SOURCE, "Play next [n]", 5, 20);
  quickDjMenu.addButton(BUTTON_SHUFFLE, "Shuffle [s]", 5, 35);
  quickDjMenu.addButton(BUTTON_UNCROSS_ALL, "Uncross all [u]", 5, 50);
  quickDjMenu.addButton(BUTTON_TIDY, "Tidy up [y]", 5, 65);

  playlistMenu.set(windowWidth - 200, 50, 195, windowHeight - 100, 1);
  playlistMenu.setScrollable(true);
  playlistMenuOptions.set(windowWidth - 220, 50, 20, windowHeight - 100, 1);
  playlistMenuTime.set(windowWidth - 270, 50, 50, windowHeight - 100, 1);

  // messages
  for (int i = 0; i < MESSAGE_MAX; i++) {
    message[i].active = false;
  }

  // playlist
  for (int i = 0; i < PLAYLIST_MAX; i++) {
    playlist[i].source = -1; // not bound to a source
  }

  playlistNum = playlistLoad(playlist);

  for (int i = 0; i < playlistNum; i++) playlistOriginal[i] = playlist[i];

  for (int i = 0; i < playlistNum; i++) {
    playlistMenu.addButton(BUTTON_NONE, playlist[i].title, 5, i * 15 + 5);
  }

  for (int i = 0; i < SOURCE_MAX; i++) {
    playlistMenuOptions.addButton(BUTTON_SKIP, "", 5, i * 15 + 5);
    playlistMenuTime.addButton(BUTTON_TIME, "00:00", 5, i * 15 + 5);
  }

  messageAdd("Welcome to 3D DJ");
  char c[100];
  snprintf(c, 100, "There are %i songs in the playlist", playlistNum);
  messageAdd(c);

  messageAdd(" ");
  messageAdd("Press h for help");


  // music

  // note: playFile() will be called from playlistNext(), which is called from
  // sourceProcess() because the source isn't initialised. This will initialise
  // it and also fill the buffer with sound and play (i.e. just a split second,
  // it will not be update()'ed because it's paused) to check for errors with
  // file. As such, the sources should be positioned out of hearing range, or
  // playFile should be set not to play if source is paused.

  // binaural
  binaural.enabled = false;
  /*binaural.x = 10;
  binaural.y = 0;
  binaural.z = -10;
  gettimeofday(&binaural.timeStart, NULL);
  // TODO test thing
  //ogg[1].setPitch(1.002); coming from right
  ogg[1].setPitch(1.005, false);
  */

  for (int i = 0; i < SOURCE_MAX; i++) {
    ogg[i].setSourceId(i);
    if (!binaural.enabled) {
      if (i % 2 == 0) ogg[i].setPosition(-1000, 0, 0);
      else ogg[i].setPosition(1000, 0, 0);
    }else{
      // position one source each side of listener for binaural sound
      if (i % 2 == 0) ogg[i].setPosition(-2.29, 0, 0);
      else ogg[i].setPosition(2.29, 0, 0);
    }
  }

}

void kill()
{
  for (int i = 0; i < SOURCE_MAX; i++)
    if (ogg[i].getSourceInitialised()) ogg[i].release();
  soundMan.killALData();
  if (window > -1) {
    if (windowMode == WINDOW_MODE_GAMEMODE_1024 || windowMode == WINDOW_MODE_GAMEMODE_1280) {
      glutLeaveGameMode();
    }else{
      glutDestroyWindow(window);
    }
  }
}

void signalProcess(int s)
{
  cout << "Signal " << s << " received." << endl;
  exit(0);
}

void processCLAs(int argc, char* argv[])
{
  struct option options[] = {
    {"fullscreen", no_argument, &windowMode, WINDOW_MODE_FULLSCREEN},
    {"gamemode1024", no_argument, &windowMode, WINDOW_MODE_GAMEMODE_1024},
    {"gamemode1280", no_argument, &windowMode, WINDOW_MODE_GAMEMODE_1280},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
  };

  int optionIndex = 0;
  int c = 0;

  // returns EOF (-1) when reaches end of CLAs
  while ((c = getopt_long(argc, argv, "hfgG", options, &optionIndex)) != EOF) {
    switch (c) {
      case 'h':
        cout << "Usage: ./main [options]" << endl;
        cout << "Options:" << endl;
        cout << "\t-f  --fullscreen" << endl;
        cout << "\t\tOpen in fullscreen mode" << endl;
        exit(0);
        break;
      case 'f':
        windowMode = WINDOW_MODE_FULLSCREEN;
        break;
      case 'g':
        windowMode = WINDOW_MODE_GAMEMODE_1024;
        break;
      case 'G':
        windowMode = WINDOW_MODE_GAMEMODE_1280;
        break;
    }
  }
}

int main(int argc, char* argv[])
{
  // seems buggy (e.g. alSourceDelete throws INVALID_OPERATION meaning there is no current context)
  // so do this manually (i.e. in sound manager/ogg_stream constructor)
  // alutInit(&argc, argv);

  glutInit(&argc, argv);

  atexit(kill);
  signal(SIGINT, signalProcess);

  processCLAs(argc, argv);

  glutInitDisplayMode (GLUT_DOUBLE | GLUT_DEPTH);

  if (windowMode == WINDOW_MODE_NORMAL) {
    glutInitWindowPosition(200,30);
    glutInitWindowSize(800, 600);
    window = glutCreateWindow ("Music");
  }else if (windowMode == WINDOW_MODE_FULLSCREEN) {
    // full screen
    glutInitWindowPosition(0,0);
    glutInitWindowSize(800, 600);
    window = glutCreateWindow ("Music");
    glutFullScreen();
  }else if (windowMode == WINDOW_MODE_GAMEMODE_1024) {
    glutGameModeString("1024x768:24@60");
    window = glutEnterGameMode();
  }else if (windowMode == WINDOW_MODE_GAMEMODE_1280) {
    glutGameModeString("1280x1024:24@60");
    window = glutEnterGameMode();
  }

  init();

  //int a = alGetInteger(AL_DISTANCE_MODEL);
  //cout << "a: " << a << ", DIST: " << AL_INVERSE_DISTANCE_CLAMPED << endl;

  glutDisplayFunc  (display);
  glutReshapeFunc  (reshape);
  glutIdleFunc     (idle);
  glutKeyboardFunc (keyboard);
  glutKeyboardUpFunc (keyboardUp);
  glutSpecialFunc (specialKeyPress);
  glutSpecialUpFunc (specialKeyUp);
  glutPassiveMotionFunc( mousePassiveMove );
  glutMotionFunc( mouseActiveMove );
  glutMouseFunc( mousePress );

  //playFile(ogg[0], argv[1], 10, 0, 5, 0, 0, 0);
  //if (numFiles > 1) playFile(ogg[1], argv[2], -10, 0, 5, 0, 0, 0);
  
  glutMainLoop();

  // done manually with atexit();
  //alutExit();

  return 0;
}

