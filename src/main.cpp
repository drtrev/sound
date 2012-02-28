#include "clientcontrol.h"
#include "getopt_basic.h"
#include "servercontrol.h"

using std::cout;
using std::cerr;
using std::endl;

namespace gob = getopt_basic;

Controller *controller;

void catchInterrupt(int sig)
{
  controller->stop();
  if (sig == SIGABRT) cout << "Program aborted." << endl;
  exit(0);
}

void processCLAs(int argc, char** argv, Args &args)
{
  struct gob::option options[] = {
    {"dontGrab", no_argument, 0, 'd'},
    {"fullscreen", no_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'},
    {"ipaddress", required_argument, 0, 'i'},
    {"nowindow", no_argument, 0, 'n'},
    {"ogg", required_argument, 0, 'o'},
    {"quiet", no_argument, 0, 'q'},
    {"server", no_argument, 0, 's'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

  int optionIndex = 0;
  int c = 0;

  // returns EOF (-1) when reaches end of CLAs
  while ((c = gob::getopt_basic(argc, argv, "dfhi:no:qsv", options, &optionIndex)) != EOF) {
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
        cout << "  -i  --ipaddress" << endl;
        cout << "    ip address of server" << endl;
        cout << "  -n  --nowindow" << endl;
        cout << "    no window" << endl;
        cout << "  -o  --ogg" << endl;
        cout << "    sound file" << endl;
        cout << "  -q  --quiet" << endl;
        cout << "    less verbose" << endl;
        cout << "  -s  --server" << endl;
        cout << "    run server" << endl;
        cout << "  -v  --verbose" << endl;
        cout << "    more verbose" << endl;
        exit(0);
        break;
      case 'i':
        args.ip = gob::optarg;
        break;
      case 'n':
        args.graphicsActive = false;
        break;
      case 'o':
        args.soundFile = gob::optarg;
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
        cerr << "For usage instructions use './sound -h' or './sound --help'" << endl;
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

