#include "clientcontrol.h"
#include "graphicsopengl.h"

#ifdef _MSC_VER
  #include <windows.h> // for winbase
  #include <winbase.h> // for Sleep
#endif

Clientcontrol::Clientcontrol()
{
  graphics = NULL;

  keys = 0, keysOld = 0;
  SERV = false;
  //input = new InputX11; if use this then delete in gameover

  myId = -1;
}

Clientcontrol::~Clientcontrol()
{
  if (graphics != NULL) {
    graphics->kill();
    delete graphics;
    std::cout << "Deleted graphics." << std::endl;
  }
}

void Clientcontrol::init(Args &args)
{
  initShared(args.verbosity, args.fullscreen); // init everything shared between client and server

  //if (args.graphicsActive) {
    graphicsActive = true;
    graphics = new GraphicsOpenGL;
    out << VERBOSE_NORMAL << "Initialising graphics...\n";
    graphics->init(out, graphics->makeWindowInfo(0, 0, 100, 100, true, true, 60, 24, args.fullscreen, "Title"),
          "/usr/share/fonts/bitstream-vera/Vera.ttf", 42);
  //}else graphicsActive = false;

  client.init(out, args.port, net.getFlagsize(), net.getUnitsize());

  out << VERBOSE_QUIET << "Using ip address: " << args.ip << '\n';

  if (!client.openConnection(args.ip.c_str())) {
    stop();
    std::cerr << "Could not connect to server at address: " << args.ip << std::endl;
    exit(1);
  }

  if (!args.dontGrab) input.grab();

  out << VERBOSE_QUIET << "Finished init.\n";
}

void Clientcontrol::stop()
{
  if (input.getGrabbed()) input.release(); // release keyboard

  stopShared();
}

void Clientcontrol::go()
  // main loop
{
  // client stats
  timer.update(); // to set current time for stats
  timeval statProgramStart = timer.getCurrent();

  // loop frequencies in Hz (calls per second)
  // these are not shared (i.e. not in controller.cpp) - might want them different on client and server
  // (not sure why, but might!) - also most aren't relevant to server
  int inputfreq = 20; // if too low then press and release will be processed in same loop and key will be ignored
  int networkfreq = 30; // if too low then graphics appear jerky without dead reckoning
  int physicsfreq = 100;
  int graphicsfreq = 60; // was 60
  int soundfreq = 40;
  int transferfreq = 10;

  int inputdelay = (int) round(1000000.0/inputfreq); // delay in microseconds
  int networkdelay = (int) round(1000000.0/networkfreq);
  int physicsdelay = (int) round(1000000.0/physicsfreq);
  int graphicsdelay = (int) round(1000000.0/graphicsfreq);
  int sounddelay = (int) round(1000000.0/soundfreq);
  int transferdelay = (int) round(1000000.0/transferfreq);

  timeval inputtime, networktime, physicstime, graphicstime, soundtime, transfertime;

  // timer updated above
  inputtime = timer.getCurrent();
  networktime = timer.getCurrent();
  physicstime = timer.getCurrent();
  graphicstime = timer.getCurrent();
  soundtime = timer.getCurrent();
  transfertime = timer.getCurrent();

  while (!(keys & KEYS_QUIT) && client.getConnected()) {
    timer.update();

    doloop(inputdelay, inputtime, &Clientcontrol::inputloop);
    //doloop(networkdelay, networktime, &Clientcontrol::networkloop);
    //doloop(physicsdelay, physicstime, &Clientcontrol::physicsloop);
    //if (graphicsActive) doloop(graphicsdelay, graphicstime, &Clientcontrol::graphicsloop);
    //doloop(sounddelay, soundtime, &Clientcontrol::soundloop);
    //doloop(transferdelay, transfertime, &Clientcontrol::transferloop);

#ifdef _MSC_VER
	Sleep(1); // bit longer
#else
    usleep(1000);
#endif
  }
}

void Clientcontrol::doloop(int delay, timeval &lasttime, void (Clientcontrol::*loopPtr) ())
{
  timeval elapsed;

  elapsed = timer.elapsed(lasttime);

  if (elapsed.tv_sec > 0 || elapsed.tv_usec > delay) {
    lasttime = timer.getCurrent();
    sync = elapsed.tv_sec + elapsed.tv_usec / 1000000.0;
    (*this.*loopPtr)();
  }
}

void Clientcontrol::inputloop()
{
  int mousexrel = 0, mouseyrel = 0;
  static int oldmousexrel = 0, oldmouseyrel = 0; // TODO remove static, make member vars
  if (input.getGrabbed()) keys = input.check(keys, mousexrel, mouseyrel, graphics->getWindowWidth(), graphics->getWindowHeight());

  if (keys != keysOld) {
    // if keys have changed, send them to server

    keysOld = keys;

    // make unit
    unit.keys.flag = UNIT_KEYS;
    unit.keys.id = myId;
    unit.keys.bits = keys;

    //net.addUnit(unit, client);

  }
}

void Clientcontrol::process(Unit unit)
//void Curses::process(Unit unit, Client &client, Server &server, Talk &talk, int keyset[])
  // process data unit
{
  switch (unit.flag) {
    case UNIT_ASSIGN:
      myId = unit.assign.id;
      out << VERBOSE_NORMAL << "Received my client ID: " << myId << '\n';
      users++;
      if (myId < 0) {
        out << VERBOSE_LOUD << "Error with assigned ID: " << myId << '\n';
      }
      break;
    case UNIT_AUDIO:
      break;
    case UNIT_GENERIC:
      break;
    case UNIT_LOGOFF:
      if (users > 0) {
        if (myId == unit.logoff.id) {
          out << VERBOSE_LOUD << "Error, server says I logged off!\n";
        }else{
          if (myId > unit.logoff.id) myId--;
          //for (int i = unit.logoff.id; i < users - 1; i++) {
            //player[i] = player[i+1]; // when new users logon, they are reset in curses.cpp, see checkNewConnections
          //}
        }
        users--;
      }
      else out << VERBOSE_LOUD << "Error, logoff received but no one left!\n";
      break;
    case UNIT_NEWCLIENT:
      if (unit.newclient.id > -1 && unit.newclient.id < MAX_CLIENTS) {
        users++;
      }else{
        out << VERBOSE_LOUD << "Invalid ID for new client\n";
      }
      break;
    case UNIT_POSITION:
      break;
    case UNIT_TRANSFER:
      break;
    case UNIT_TRANSFERFIN:
      break;
    case UNIT_TRANSFERREQ:
      break;
    default:
      out << VERBOSE_LOUD << "Error, flag not found: " << unit.flag << '\n';
  }
}

void Clientcontrol::networkloop()
{
  /*client.doSelect();
  unit = client.recvDataUnit(net);

  while (unit.flag > -1) {
    process(unit);
    client.doSelect();
    unit = client.recvDataUnit(net);
  }

  client.sendData();*/
}

void Clientcontrol::physicsloop()
{
}

void Clientcontrol::graphicsloop()
{
  // important the way this is done (if using curses):
  // graphics: draw->refresh->clear, then status changes in other loops
  // this means output is cleared in the correct location, and avoids problems of
  // clear->draw->refresh, then status change: deactivate which would mean deactivated
  // objects are not cleared

  if (myId > -1 && myId < players) {

    graphics->drawStart();

    graphics->drawStop();

    out.refreshScreen();
    graphics->refresh();

    // for curses
    //for (int i = 0; i < users; i++) user[i].clear();
  }

}

void Clientcontrol::soundloop()
{
  //if (talk.getCapturing()) talk.capture(myId, net, client);
  //if (soundDev.checkPlayContext()) talk.update();
}

void Clientcontrol::transferloop()
// deal with file transfer
{
}
