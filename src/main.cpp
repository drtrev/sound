#include "soundmanager.h"
#include "ogg.h"

using namespace std;

#define SOURCE_MAX 10
SoundManager soundMan;
ogg_stream ogg[SOURCE_MAX];

bool playFile(ogg_stream &ogg, const char* filename, float px, float py, float pz, float sx, float sy, float sz)
{
  try {
    ogg.open_inmemory(filename);
    //ogg.open(filename);
  }catch(string error){
    cerr << "Error" << endl;
    //char c[50];
    //snprintf(c, 50, "Could not open file: %s", filename);
    //messageAdd(c);
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
    //char c[50];
    //snprintf(c, 50, "Error: %s", error.c_str());
    //messageAdd(c);
    return false; // failure
  }

  // TODO quick hack for binaural... it starts paused
  /*if (!binaural.enabled) {
    if (!ogg.playing()) {
      cout << "Releasing ogg file and exiting..." << endl;
      ogg.release();
      exit(1);
    }
  }*/

  return true; // success
}

int sourceProcess(ogg_stream &ogg)
{
  // play next part of stream
  //if (ogg.getPaused()) return 0;

  if (!ogg.update()) {
    cout << "Ogg finished." << endl;
    ogg.release();
    return 1;
  }
  return 0;
}

int main(int argc, char** argv)
{
  playFile(ogg[0], argv[1], 10, 0, 5, 0, 0, 0);

  while(1) {
    if (sourceProcess(ogg[0]) == 1) break;
    usleep(10000);
  }

  return 0;
}

