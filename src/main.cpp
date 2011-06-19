#include "clientcontrol.h"
#include "getopt_basic.h"
#include "ogg.h"
#include "servercontrol.h"

using std::cout;
using std::cerr;
using std::endl;

namespace gob = getopt_basic;

#define SOURCE_MAX 10
ogg_stream ogg[SOURCE_MAX];

Controller *controller;

void catchInterrupt(int sig)
{
  controller->stop();
  if (sig == SIGABRT) cout << "Program aborted." << endl;
  exit(0);
}

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

void processCLAs(int argc, char** argv, Args &args)
{
  struct gob::option options[] = {
    {"dontGrab", no_argument, 0, 'd'},
    {"fullscreen", no_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'},
    {"ipaddress", required_argument, 0, 'i'},
    {"nowindow", no_argument, 0, 'n'},
    {"quiet", no_argument, 0, 'q'},
    {"server", no_argument, 0, 's'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

  int optionIndex = 0;
  int c = 0;

  // returns EOF (-1) when reaches end of CLAs
  while ((c = gob::getopt_basic(argc, argv, "dfhi:nqsv", options, &optionIndex)) != EOF) {
    switch (c) {
      case 'd': // don't grab keyboard
        args.dontGrab = true;
        break;
      case 'f':
        args.fullscreen = true;
        break;
      case 'h': // help
        cout << "Usage: ./net [options]" << endl;
        cout << "Options:" << endl;
        cout << "  -d  --dontGrab" << endl;
        cout << "    don't grab keyboard" << endl;
        cout << "  -f  --fullscreen" << endl;
        cout << "    open in fullscreen mode" << endl;
        cout << "  -h  --help" << endl;
        cout << "    display this help" << endl;
        exit(0);
        break;
      case 'i':
        args.ip = gob::optarg;
        break;
      case 'n':
        args.graphicsActive = false;
        break;
      case 'q':
        if (args.verbosity == VERBOSE_LOUD) args.verbosity = VERBOSE_NORMAL; // got v as well
        else args.verbosity = VERBOSE_QUIET;
        break;
      case 's':
        args.serv = true; // can't use SERVER cos that's defined in server.h
        break;
      case 'v':
        if (args.verbosity == VERBOSE_QUIET) args.verbosity = VERBOSE_NORMAL; // got q as well
        else args.verbosity = VERBOSE_LOUD;
        break;
      case '?': // error
        cerr << "For usage instructions use './geo -h' or './geo --help'" << endl;
        break;
    }
  }

}

int main(int argc, char** argv)
{
  signal(SIGINT, &catchInterrupt);
  signal(SIGABRT, &catchInterrupt);

  Args args;
  processCLAs(argc, argv, args);

  if (!args.serv) controller = new Clientcontrol;
  else controller = new Servercontrol;

  controller->init(args);
  controller->go();
  controller->stop();

  //playFile(ogg[0], argv[1], 10, 0, 5, 0, 0, 0);

  //while(1) {
    //if (sourceProcess(ogg[0]) == 1) break;
    //usleep(10000);
  //}

  return 0;
}

