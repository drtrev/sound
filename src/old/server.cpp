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

/*
 * server.cc
 *
 * A server program for the CVE.
 * Accepts new connections, passes on messages and sends out its own
 * messages for initialising gravity and creating new blocks.
 *
 * Trevor Dodds, 2005
 */

#include <iostream>
#include <fstream>
#include <errno.h>

#ifdef _LINUX_
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h> // for gettimeofday
#else
#include <winsock.h>
#include <time.h>
#endif

#include "network.h"
#include "buffer.h"

using namespace std;

//#define TRAFFIC_ANALYSIS_COUNT 1

int listenSock;  // listen on sock_fd, new connection on clSocks
int udpSock; // socket for receiving UDP stuff
Buffer sendBuffer[MAX_CLIENTS];
Buffer sendBufferUdp[MAX_CLIENTS];
Buffer serverBuffer;
Buffer serverBufferFile; // for playback file - seperate so you can still send stuff to server
struct sockaddr_in my_addr;    // my address information
struct sockaddr_in my_addr_udp;    // my address information
struct sockaddr_in their_addr; // connector's address information
struct sockaddr_in clSocksUdp[MAX_CLIENTS];
int sin_size;
int sockoptyes=1;
int numHosts=0;

int clCount = 0; // client counter (how many clients we have)
int clSocks[MAX_CLIENTS]; // client sockets
char id[MAX_CLIENTS]; // client ids
int nextId = SERVER_ID + 1;
int maxSock = 0;
//struct timeval timeout;
fd_set readSocks;
fd_set writeSocks;

//clock_t trafficAnalysisTimer; // for analysing traffic
//int traffic; // number of bytes received

//clock_t serverStartTime;

#ifndef _LINUX_
    //bool winSockInit;
    WSADATA wsaData;
#endif

// for recording data
ofstream recordFile;
ifstream playbackFile;
string playbackFilename;
bool recording = false;
bool playback = false, justStartedPlayback = false, playbackPaused = false, playbackFast = false, playbackInstant = false;
int filesize = 0;
// was using (float) ((clock() - serverStartTime) / (float) CLOCKS_PER_SEC)
// to determine current time, but CLOCKS_PER_SEC is a constant and so if the process is
// running with a different number of CLOCKS_PER_SEC on playback then it will playback at a different speed
struct timeval timeElapsed; // for recording, the time elapsed since recording started. for playback, the time elapsed to wait for
struct timeval timeStart;
struct timeval timeLastGroupUpdate; // update groups on regular intervals
//struct timeval timeSendData;

bool waitingForMoreDataFromFile = false;

// fast trig stuff
float Trig::cosValues[721];
float Trig::sinValues[721];
float Trig::asinValues[121];

// Output traffic for analysis
/*void analyseTraffic()
{
  //cout << traffic << endl;
  
  // reset traffic count
  traffic = 0;

  // reset timer
  trafficAnalysisTimer = clock() + (long int) (TRAFFIC_ANALYSIS_COUNT * CLOCKS_PER_SEC);
}*/

// The number of seconds elapsed since the server started
//
// Returns:
//   an integer representing the number of seconds elapsed since the server started
/*int secondsElapsed()
{
  return (int) ((clock() - serverStartTime) / CLOCKS_PER_SEC);
}*/

char getNextId()
{
  // The ID is not taken as the array
  // element number in clSocks because the array elements will be shifted
  // upon a connection close by one of the clients
  // ID cannot be null or ASCII_START_DATA because they are reserved for special use
  if (nextId > 127) nextId = SERVER_ID + 1;
  return (char) nextId++;
}

// Initialise server
void serverInit()
{
#ifndef _LINUX_
  if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
    fprintf(stderr, "WSAStartup failed.\n");
    exit(1);
  }
  else
  {
    cout << "WSAStartup successful" << endl;
//    winSockInit = true;
  }
#endif
  gettimeofday(&timeStart, NULL);
  timeElapsed.tv_sec = 0;
  timeElapsed.tv_usec = 0;

  if ((listenSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    cerr << "Server::serverInit(): error setting up listenSock with socket()" << endl;
    perror("socket");
    exit(1);
  }
  if ((udpSock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    cerr << "Server::serverInit(): error setting up listenSock with socket()" << endl;
    perror("socket");
    exit(1);
  }

#ifdef _LINUX_
  if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &sockoptyes, sizeof(int)) == -1) {
#else
  if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*) &sockoptyes, sizeof(int)) == -1) {
#endif
    cerr << "Server::serverInit(): error setting up listenSock with setsockopt()" << endl;
    perror("setsockopt");
    exit(1);
  }

  my_addr.sin_family = AF_INET;         // host byte order
  my_addr.sin_port = htons(PORT);       // short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

  my_addr_udp.sin_family = AF_INET;         // host byte order
  my_addr_udp.sin_port = htons(PORT+1);       // short, network byte order
  my_addr_udp.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr_udp.sin_zero), '\0', 8); // zero the rest of the struct

  // bind the socket to a port
  if (bind(listenSock, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
    cerr << "Server::serverInit(): error setting up listenSock with bind()" << endl;
    perror("bind");
    exit(1);
  }
  // TODO should I be using a seperate port number here?
  if (bind(udpSock, (struct sockaddr *)&my_addr_udp, sizeof(struct sockaddr)) == -1) {
    cerr << "Server::serverInit(): error setting up udpSock with bind()" << endl;
    perror("bind");
    exit(1);
  }

  if (listen(listenSock, BACKLOG) == -1) {
    cerr << "Server::serverInit(): error setting up listenSock with listen()" << endl;
    perror("listen");
    exit(1);
  }

  //trafficAnalysisTimer = clock() + (long int) (TRAFFIC_ANALYSIS_COUNT * CLOCKS_PER_SEC);

}

void intToChar(int i, unsigned char c[])
{
  c[0] = (i & 255<<24)>>24;
  c[1] = (i & 255<<16)>>16;
  c[2] = (i & 255<<8)>>8;
  c[3] = i & 255;
}

int charToInt(unsigned char c[])
{
  return (c[0]<<24) + (c[1]<<16) + (c[2]<<8) + c[3];
}

// TODO float is just 4 bytes so can do this the same as for int (i.e. without losing precision)
void floatToChar(float f, unsigned char c[])
{
  // truncating to three decimal places, allowing -2.4 mill to 2.4 mill
  int i = (int) (f * 100);
  intToChar(i, c);
}

float charToFloat(unsigned char c[])
{
  return charToInt(c) / 100.0;
}

// initialises a string to contain network data header of size 3
string makeNetworkData(const int id, const int flag)
{
  string networkData;
  networkData = ASCII_START_DATA;
  networkData += id;
  networkData += flag;
  return networkData;
}

int getHumanNum(int clientId)
{
  int humanNum = -1;
  for (int i = 0; i < MAX_HUMANS; i++) {
    if (humans[i].getActive() && humans[i].getClientId() == clientId) humanNum = i;
  }
  return humanNum;
}

void displayGroups()
{
  // output groups with timestamp
  for (int i = 0; i < MAX_GROUPS; i++) {
    cout << "[" << timeElapsed.tv_sec / 60 << ":" << timeElapsed.tv_sec - timeElapsed.tv_sec / 60 * 60 << "] Group " << i << ": ";
    for (int j = 0; j < groups[i].getSize(); j++) {
      if (j > 0) cout << ", ";
      cout << humans[getHumanNum(groups[i].getMember(j))].getName();
    }
    cout << endl;
  }

  /*int h = 0;
  cout << "Group output:" << endl;
  for (int i = 0; i < MAX_GROUPS; i++) {
    if (groups[i].getSize() > 0) {
      cout << i << endl;
      for (int member = 0; member < groups[i].getSize(); member++) {
        h = getHumanNum(groups[i].getMember(member));
        if (humans[h].inParent(i)) cout << "  ";
        cout << humans[h].getName() << endl;
      }
    }
  }

  h = getHumanNum(50); // spectator
  if (h > -1) {
    cout << "Spectator group: " << humans[h].getGroup() << endl;
    cout << "Spectator parents: ";
    for (int i = 0; i < humans[h].getNumParents(); i++) cout << humans[h].getParent(i) << ", ";
    cout << endl;
  }*/
}

void getTimeElapsed(struct timeval *t)
{
  gettimeofday(t, NULL);
  t->tv_sec -= timeStart.tv_sec;
  t->tv_usec -= timeStart.tv_usec;

  if (t->tv_usec < 0) {
    // take off a second and add a million microseconds
    t->tv_sec--;
    t->tv_usec += 1000000;
  }
}

void saveTimeElapsed()
{
  string networkData = makeNetworkData(SERVER_ID, FLAG_TIME_ELAPSED);
  unsigned char c[4];

  getTimeElapsed(&timeElapsed);
  //gettimeofday(&timeElapsed, NULL);
  //timeElapsed.tv_sec -= timeStart.tv_sec;
  // treat the start time as if it started with 0 microseconds
  //timeElapsed.tv_usec -= timeStart.tv_usec;

  intToChar((int) timeElapsed.tv_sec, c);
  networkData += c[0]; networkData += c[1];
  networkData += c[2]; networkData += c[3];
  intToChar((int) timeElapsed.tv_usec, c);
  networkData += c[0]; networkData += c[1];
  networkData += c[2]; networkData += c[3];
  recordFile.write(networkData.c_str(), networkData.length());
}

void clearBuffer(Buffer &buf)
{
  int bufferSize = buf.getDataOnBuffer();
#ifdef _LINUX_
  char removeData[bufferSize];
#else
  char removeData[5000];
#endif
  
  if (bufferSize > 0) {
    if (buf.read(removeData, bufferSize) != bufferSize) cerr << "clearBuffer(): Error reading from buffer." << endl;
  }
}

// Receive data from clients
void receiveData()
{
  // TODO server could receive partial data from one client, send to others, then
  // receive data from another client, send to others, before receiving the rest
  // from the first client, which would therefore break up data units.
  // However, small packet size means that all data should be received at once.
  char buf[MAX_RECV_DATA]; // for receiving data
  int bytesReceived = 0;
  int bytesStored = 0;

  // go through all the sockets and store the data
  for (int client = 0; client < clCount; client++) {
    
    // is the socket set as readable?
    if (FD_ISSET(clSocks[client], &readSocks)) {

      // read the socket
      // set length as MAX_RECV_DATA-1 to allow for element after last to be set to null
      // edit: last element no longer set to null
      //int intData;
      bytesReceived = recv(clSocks[client], buf, MAX_RECV_DATA, 0);
      //bytesReceived = recv(clSocks[client], intData, sizeof(int), 0);
      //intToChar(intData, buf);
      
      // check for errors
      if (bytesReceived < 1) {

        bool closeSocket = false;
        
        // -1 is an error, could be 'connection reset'
        // 0 means connection has been closed by remote side
        if (bytesReceived == -1) {
          int errorNum = errno;
          cerr << "Server::receiveData(): receive error" << endl;
          perror("perror");
          // was the error 'connection reset'?
#ifdef _LINUX_
          if (errorNum == ECONNRESET)
#else
          if (errorNum == WSAECONNRESET)
#endif
          {
            cerr << "It's ok though. Detected errno as connection reset" << endl;
            cerr << "Closing socket" << endl;
            closeSocket = true;
          }
        }else{
          cerr << "No bytes received indicating connection was closed" << endl;
          cerr << "Closing socket" << endl;
          closeSocket = true;
        }

        if (closeSocket) {
#ifdef _LINUX_
          if (close(clSocks[client]) == -1)
#else
          if (closesocket(clSocks[client]) == -1)
#endif
          {
            cerr << "Server::receiveData(): close error" << endl;
            perror("perror close()");
          }
          clSocks[client] = -1; // removed from array below
        }
        
      }else{

        //
        // we've got at least one byte
        // queue the received data ready for sending
        //

        // for monitoring network traffic
        //traffic += bytesReceived;

        // set element after last used to null
        //buf[bytesReceived] = 0;
        //cout << "bytesReceived: " << bytesReceived << endl;

        //cout << "Buffer contents: " << buf << endl;

        // allow for every char being an ASCII_START_DATA plus null on the end
        // letting client add id's now instead
        //char bufWithIds[bytesReceived * 2];
//#ifdef _LINUX_
//        char bufWithIds[bytesReceived];
//#else
//        char bufWithIds[5000];
//#endif
        
        // add id of client after each data start
        //int j = 0;
        //for (int i = 0; i < bytesReceived; i++) {
        //  bufWithIds[j++] = buf[i];
          //if (buf[i] == ASCII_START_DATA) bufWithIds[j++] = id[client];
          // Could pick up ASD within a data unit!!
          // let client add id's instead!
        //}

        //cout << "buffWithIds contents: " << bufWithIds << endl;
        //cout << "buffWithIds size: " << j << endl;

        // add to server buffer
        bytesStored = serverBuffer.write(buf, bytesReceived);
        if (bytesStored != bytesReceived) {
          cerr << "Server::receiveData(): overflow of serverBuffer" << endl;
        }

        // save data if recording
        if (recording) {
          saveTimeElapsed();

          // how long since last group update
          struct timeval t;
          gettimeofday(&t, NULL);
          t.tv_sec -= timeLastGroupUpdate.tv_sec;
          t.tv_usec -= timeLastGroupUpdate.tv_usec;

          if (t.tv_usec < 0) {
            // take off a second and add a million microseconds
            t.tv_sec--;
            t.tv_usec += 1000000;
          }
          
          // every 5 seconds write groups
          if (t.tv_sec > 4) {
            string networkData = "";
            unsigned char c[4];
            for (int i = 0; i < MAX_HUMANS; i++) {
              if (humans[i].getActive()) {
                networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_UPDATE);
                intToChar(humans[i].getClientId(), c);
                networkData += c[0]; networkData += c[1];
                networkData += c[2]; networkData += c[3];
                intToChar(humans[i].getGroup(), c);
                networkData += c[0]; networkData += c[1];
                networkData += c[2]; networkData += c[3];
                //cout << "group: " << humans[i].getGroup() << " charToInt: " << charToInt(c) << endl;
                for (int p = 0; p < 5; p++) {
                  if (p < humans[i].getNumParents()) {
                    intToChar(humans[i].getParent(p), c);
                    networkData += c[0]; networkData += c[1];
                    networkData += c[2]; networkData += c[3];
                  }else{
                    intToChar(-1, c);
                    networkData += c[0]; networkData += c[1];
                    networkData += c[2]; networkData += c[3];
                  }
                }
                // TODO if haven't received a whole data unit then this will appear in between one
                recordFile.write(networkData.c_str(), networkData.length());

                // write to clients
                for (int i = 0; i < clCount; i++) {
                  bytesStored = sendBuffer[i].write(networkData.c_str(), networkData.length());
                  if (bytesStored != (int) networkData.length()) {
                    cerr << "Server::receiveData(): bytes stored not equal to group update length." << endl;
                  }
                }
              }
            }
            gettimeofday(&timeLastGroupUpdate, NULL);
          }
          recordFile.write(buf, bytesReceived);
        }
        
        // go through client buffers
        // append to all buffers except originating client
        for (int i = 0; i < clCount; i++) {

          // if we're not dealing with the originating client's buffer
          if (i != client) {

            // write the received data to the client's buffer
            bytesStored = sendBuffer[i].write(buf, bytesReceived);

            // if the number of bytes written is less than what was received
            // i.e. the buffer is full
            if (bytesStored != bytesReceived) {
              cerr << "Server::receiveData(): bytes stored not equal to bytes received." << endl;
              cerr << "  - Possible overflow of sendBuffer" << endl;
              cerr << "    > Client should recover using start of data ASCII char to skip forward" << endl;
            }

          }

        } // end for
          
      } // end if bytesReceived < 1

    } // end is socket readable

  } // end for


  // get rid of closed sockets
  unsigned char c[4];
  int logoutId = -1, humanNum = -1;
  string networkData = "";
  for (int i = 0; i < clCount; i++) {
    if (clSocks[i] == -1) {
      cerr << "Server::receiveData(): OK. Removed connection from clSocks[]" << endl;
      logoutId = id[i];
      // could have something written into it from another client received in the loop above
      cout << "There is " << sendBuffer[i].getDataOnBuffer() << " data still on buffer" << endl;
      for (int j = i; j < clCount-1; j++) {
        clSocks[j] = clSocks[j+1];
        sendBuffer[j] = sendBuffer[j+1];
        id[j] = id[j+1];
      }
      
      clearBuffer(sendBuffer[clCount-1]);
      clCount--;

      cout << "[" << timeElapsed.tv_sec / 60 << ":" << timeElapsed.tv_sec - timeElapsed.tv_sec / 60 * 60 << "] ";
      cout << humans[getHumanNum(logoutId)].getName() << " logged out" << endl;

      // remove client from any groups they are in
      for (int i = 0; i < MAX_GROUPS; i++) {
        groups[i].removeMember(logoutId); // removes all instances
        if (groups[i].getSize() == 1) {
          // remove remaining member
          int client = groups[i].getMember(0);
          humanNum = getHumanNum(client);
          if (humanNum == -1) cerr << "Logout: error finding human num for remaining group member (client: " << client << ")" << endl;
          else humans[humanNum].setGroup(humans[humanNum].popParentGroup());
          groups[i].clear();
        }
      }
     
      //displayGroups();

      // log them out
      networkData = makeNetworkData(SERVER_ID, FLAG_LOGOUT);
      intToChar(logoutId, c);
      networkData += c[0]; networkData += c[1];
      networkData += c[2]; networkData += c[3];

      for (int j = 0; j < clCount; j++) {

        // write the received data to the client's buffer
        bytesStored = sendBuffer[j].write(networkData.c_str(), networkData.length());

        // if the number of bytes written is less than what was received
        // i.e. the buffer is full
        if (bytesStored != 7) {
          cerr << "Server::receiveData()/logout: bytes stored not equal to 7." << endl;
          cerr << "  - Possible overflow of sendBuffer" << endl;
          cerr << "    > Client should recover using start of data ASCII char to skip forward" << endl;
        }

      } // end for

      if (recording) recordFile.write(networkData.c_str(), networkData.length());

      humanNum = getHumanNum(logoutId);
      if (humanNum == -1) cerr << "Error finding human for logoutId: " << logoutId << endl;
      else {
        cout << "Logging out " << humans[humanNum].getName() << " (client " << logoutId << ", human " << humanNum << ")" << endl;
        humans[humanNum].setActive(false);
      }

    }
  }

  if (FD_ISSET(udpSock, &readSocks)) {
    // read UDP socket
#ifdef _LINUX_
    socklen_t addr_len = sizeof(struct sockaddr);
#else
    int addr_len = sizeof(struct sockaddr);
#endif
    bytesReceived = recvfrom(udpSock, buf, MAX_RECV_DATA, 0, (struct sockaddr *) &their_addr, &addr_len);
    if (bytesReceived == -1) {
      perror("recvfrom");
    }
    if (bytesReceived > 0) {
      //gettimeofday(&timeSendData, NULL);
      //cout << "sendData: " << timeSendData.tv_sec << ":" << timeSendData.tv_usec << ", bytes: " << bytesReceived << endl;
      // go through client buffers
      // append to all buffers except originating client
      for (int i = 0; i < clCount; i++) {

        // if we're not dealing with the originating client's buffer
        if (clSocksUdp[i].sin_addr.s_addr != their_addr.sin_addr.s_addr) {
          //cout << "Writing UDP to sendBuffer[" << i << "]" << endl;

          // write the received data to the client's buffer
          bytesStored = sendBufferUdp[i].write(buf, bytesReceived);

          // if the number of bytes written is less than what was received
          // i.e. the buffer is full
          if (bytesStored != bytesReceived) {
            cerr << "Server::receiveData(): bytes stored not equal to bytes received." << endl;
            cerr << "  - Possible overflow of sendBuffer" << endl;
            cerr << "    > Client should recover using start of data ASCII char to skip forward" << endl;
          }

        }

      } // end for

      // save data if recording
      if (recording) {
        saveTimeElapsed();
        recordFile.write(buf, bytesReceived);
      }
    }
  }
}

void readPlaybackFile()
{
  char buf[MAX_RECV_DATA];
  // read data from file
  if ((int) playbackFile.tellg() != filesize) {
    int currentPos = playbackFile.tellg();
    int amountRead = (currentPos + MAX_RECV_DATA > filesize) ? filesize - currentPos : MAX_RECV_DATA;
    playbackFile.read(buf, amountRead);

    // add to server buffer
    int bytesStored = serverBufferFile.write(buf, amountRead);
    if (bytesStored != amountRead) {
      cerr << "Server::playbackData(): overflow of serverBufferFile" << endl;
    }

  }else{
    cout << "Finished reading playback file. Closing." << endl;
    playbackFile.close();
    playback = false;
  }
}

// Send some data
void sendData()
{
  // dataToSend needs to be able to hold all buffer contents plus one more
  // character for 'null' at the end of the string of data for output (testing only)
  char dataToSend[BUFFER_SIZE + 1];
  int bytesStored = 0, bytesRead = 0;
  

  // go through client buffers for active clients and send data where there is
  // some and if socket is writable
  for (int client = 0; client < clCount; client++) {

    bytesStored = sendBuffer[client].getDataOnBuffer();

    // if there's data to send and socket is writable
    if (bytesStored > 0 && FD_ISSET(clSocks[client], &writeSocks)) {

      // read all of buffer contents into dataToSend
      bytesRead = sendBuffer[client].read(dataToSend, bytesStored);
      // error check
      if (bytesRead != bytesStored) cerr << "Server::sendData(): error reading from buffer" << endl;

      // the element after the data must be set to null to end the string. Otherwise more
      // data is sent than intended

      // TODO - this doesn't necessarily send all of buffer, see sendall() on Beej's tutorial
      // note that it returns the number of bytes sent, so this can be used to send the rest later
      if (send(clSocks[client], dataToSend, bytesRead, 0) == -1) {
        cerr << "Server::sendData(): send error" << endl;
        perror("perror");
      }
      dataToSend[bytesRead] = 0; // set element after last to null for output
      //gettimeofday(&timeSendData, NULL);
      //cout << "sendData: " << timeSendData.tv_sec << ":" << timeSendData.tv_usec << endl;
      //cout << "Sending data: " << dataToSend << endl;

    } // end if there's data to send

  } // end for

  // send to UDP addresses
  for (int client = 0; client < clCount; client++) {
    bytesStored = sendBufferUdp[client].getDataOnBuffer();

    if (bytesStored > 0 && FD_ISSET(udpSock, &writeSocks)) {
      bytesRead = sendBufferUdp[client].read(dataToSend, bytesStored);
      if (bytesRead != bytesStored) cerr << "Server::sendData(): error reading from buffer" << endl;
      int n = 0, total = 0, count = 0;
      // TODO should save the rest to next loop (i.e. put back in buffer) rather than keep trying for ages
      while (total < bytesStored) {
        n = sendto(udpSock, dataToSend + total, bytesRead, 0, (struct sockaddr *) &clSocksUdp[client], sizeof(struct sockaddr));
        if (n == -1) break;
        total += n;
        bytesRead -= n;
        count++;
        if (total < bytesStored && count == 20) {
          cerr << "UDP sendto error: Tried 20 times to sendto() but failed for client: " << id[client] << endl;
        }
      }
      if (n == -1) perror("perror sendto");
      /*cout << "Sent UDP data to: ";
      cout << (int) ((unsigned char*) &clSocksUdp[client].sin_addr.s_addr)[0] << ".";
      cout << (int) ((unsigned char*) &clSocksUdp[client].sin_addr.s_addr)[1] << ".";
      cout << (int) ((unsigned char*) &clSocksUdp[client].sin_addr.s_addr)[2] << ".";
      cout << (int) ((unsigned char*) &clSocksUdp[client].sin_addr.s_addr)[3] << endl;*/
    }
  }
}

void newGroup(Human &h1, Human &h2, bool reuseGroup)
{
  int nextGroup = getNextGroup(reuseGroup); // subgroups don't reuse group numbers - add them to the end
  if (nextGroup == -1) {
    cerr << "Server::newGroup(): Error. No more groups could be allocated" << endl;
  }else{
    // TODO nextGroup doesn't have to be in sequence, as groups have Id's
    // i.e. if groups break up, their array element can be used for the next group
    // at the minute just using element of array as an ID, so could remove group Id
    // selectors and mutators from group.h??

    // tell any existing members they're not in a group anymore!
    int humanNum = 0;
    for (int i = 0; i < groups[nextGroup].getSize(); i++) {
      humanNum = getHumanNum(groups[nextGroup].getMember(i));
      if (humanNum > -1) humans[humanNum].setGroup(-1);
    }
    groups[nextGroup].clear();
    groups[nextGroup].addMember(h1.getClientId());
    groups[nextGroup].addMember(h2.getClientId());
    //groups[nextGroup].setLeader(humans[i].getClientId());
    h1.setGroup(nextGroup);
    h2.setGroup(nextGroup);
    cout << "[" << timeElapsed.tv_sec / 60 << ":" << timeElapsed.tv_sec - timeElapsed.tv_sec / 60 * 60 << "] ";
    cout << "Created " << nextGroup << ": " << h1.getName() << ", " << h2.getName() << endl;
    //cout << "Created group " << nextGroup;// << " with humans " << i << " and " << j;
    //cout << " with clients " << h1.getClientId() << " and " << h2.getClientId() << endl;
    //cout << " (" << h1.getName() << " and " << h2.getName() << ") and transmitting this..." << endl;

    // transmit this group creation across the network
    string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_NEW);
    unsigned char c[4];
    intToChar(nextGroup, c);
    networkData += (char) c[0]; networkData += (char) c[1];
    networkData += (char) c[2]; networkData += (char) c[3];
    intToChar(h1.getClientId(), c);
    networkData += (char) c[0]; networkData += (char) c[1];
    networkData += (char) c[2]; networkData += (char) c[3];
    intToChar(h2.getClientId(), c);
    networkData += (char) c[0]; networkData += (char) c[1];
    networkData += (char) c[2]; networkData += (char) c[3];
    for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());
  }
}

void joinGroup(int groupNum, Human &h)
{
  // are they already a member?
  bool found = false;
  for (int i = 0; i < groups[groupNum].getSize(); i++) {
    if (groups[groupNum].getMember(i) == h.getClientId()) found = true;
  }
  if (found) cout << "JOIN: client " << h.getClientId() << " already a member of group " << groupNum << endl;
  else {
    h.setGroup(groupNum);
    groups[groupNum].addMember(h.getClientId());

    // output timestamp and group members
    cout << "[" << timeElapsed.tv_sec / 60 << ":" << timeElapsed.tv_sec - timeElapsed.tv_sec / 60 * 60 << "] ";
    cout << h.getName() << " joined " << groupNum << ": ";
    for (int i = 0; i < groups[groupNum].getSize(); i++) {
      if (i > 0) cout << ", ";
      cout << humans[getHumanNum(groups[groupNum].getMember(i))].getName();
    }
    cout << endl;
    //cout << "client " << h.getClientId() << " (" << h.getName() << ") has been added to group " << groupNum << endl;
    //cout << "  transmitting..." << endl;
    // send this out
    string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_JOIN);
    unsigned char c[4];
    // groupNum, clientId
    intToChar(groupNum, c);
    networkData += (char) c[0]; networkData += (char) c[1];
    networkData += (char) c[2]; networkData += (char) c[3];
    intToChar(h.getClientId(), c);
    networkData += (char) c[0]; networkData += (char) c[1];
    networkData += (char) c[2]; networkData += (char) c[3];
    for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());
  }
}

int makeNewHuman(int clientId)
{
  int humanNum = -1;
  for (int i = 0; i < MAX_HUMANS; i++) {
    if (!humans[i].getActive()) {
      humans[i].set(0, 0, 0, 0, 0, 0, nextHumanGlobalId++, clientId);
      cout << "Made human number: " << i << ", clientId: " << clientId << endl;
      humanNum = i;
      // request name for this human
      //string networkData = makeNetworkData(FLAG_NAME_REQUEST);
      //client.sendData(networkData.c_str(), networkData.length());
      break;
    }
  }
  return humanNum;
}

void processDataUnit(char du[], const int length, bool sendOut)
{
  // first part is ID of sender, then flag, then the data starts at element 2
  // length is total length of du including ID and flag
  int messageSender = (int) du[0];
  char flag = du[1];

  // if in playback mode, send it out to clients to process
  // only sendOut if it's the data read from the file,
  // otherwise it will already be sent out to all except the originating
  // client
  //
  // if fast forwarding past audio then notify with server output
  //if (playback && sendOut && playbackFast && flag == FLAG_AUDIO) cout << "Fast forwarding audio" << endl;
  if (playback && sendOut && (flag != FLAG_AUDIO || !playbackFast)) {
    int bytesStored = 0;
    char c[1];
    c[0] = ASCII_START_DATA;

    // go through client buffers
    // append to all
    // TODO this sends out FLAG_TIME_ELAPSED too which isn't necessary
    // and so playback takes more bandwidth than original recording
    for (int i = 0; i < clCount; i++) {

      // write the received data to the client's buffer
      bytesStored = sendBuffer[i].write(c, 1);
      if (bytesStored != 1) cerr << "processDataUnit(): error writing ASCII_START_DATA for playback mode" << endl;

      bytesStored = sendBuffer[i].write(du, length);

      // if the number of bytes written is less than what was received
      // i.e. the buffer is full
      if (bytesStored != length) {
        cerr << "Server::processDataUnit(): bytes stored not equal to bytes received." << endl;
        cerr << "  - Possible overflow of sendBuffer" << endl;
        cerr << "    > Client should recover using start of data ASCII char to skip forward" << endl;
      }

    } // end for
  }
  
  // can't initialise things in switch
  // unless they're in a block (e.g. if statement)
  string tempStr = "";
  float posX = 0.0, posY = 0.0, posZ = 0.0, angleX = 0.0, angleY = 0.0;

  int groupNum = 0, clientId = 0, view = 0;

  int humanNum = -1, humanNum2 = -1;
  if (messageSender != SERVER_ID) {
    humanNum = getHumanNum(messageSender);
    if (humanNum == -1) {
      if (flag != FLAG_NAME) {
        cerr << "ERROR: processDataUnit(): humanNum not found for messageSender " << messageSender;
        cerr << " and flag is not NAME it's intval: " << (int) flag << endl;
        if (messageSender > 49) cerr << "Assuming it's just a spectator that logged out because messageSender is > 49" << endl;
        // TODO spectator sends position data, and logs off, and splitData has been
        // waiting for elapsed time, and then processes position data, so then
        // there is no human left
        flag = FLAG_NONE;
      }else{
        // not found, so make new human with messageSender
        humanNum = makeNewHuman(messageSender);
        cout << "Flag is: " << flag << endl;
        if (humanNum == -1) {
          cerr << "processDataUnit(): error making new human for messageSender: " << messageSender << endl;
          humanNum = 0;
          flag = FLAG_NONE;
        }
      }
    }
  }

  switch (flag) {
    case FLAG_NONE: // do nothing
      break;
    case FLAG_AUDIO:
      //posX = humans[humanNum].getX(); posY = humans[humanNum].getY(); posZ = humans[humanNum].getZ();
      //soundMan.receiveAudio(humans[humanNum].getId(), &du[2], length - 2, posX, posY, posZ);
        //cout << "Received all audio data: playing once" << endl;
        //soundMan.playRelative(7, 0.0, 0.0, 0.0);
      playbackInstant = true; // TODO this is never executed because audio is transmitted on UDP!!
      break;
    case FLAG_BULLET:
      // make sure human is at target position
      //humans[humanNum].setToPosition();
      //humans[humanNum].fire(level, bullets, charToInt((unsigned char*) &du[2]), liftRegions, doorRegions, soundMan, true);
      break;
    case FLAG_GROUP_ADD_PARENT:
      cerr << "Error - received add parent flag." << endl;
      break;
    case FLAG_GROUP_NEW:
      cerr << "Error - received group new flag." << endl;
      break;
    case FLAG_GROUP_LEAVE:
      /*groupNum = charToInt((unsigned char*) &du[2]);
      if (groupNum < 0 || groupNum > MAX_GROUPS -1) cerr << "processDataUnit(): Error. GroupNum out of range: " << groupNum << endl;
      else {
        groups[groupNum].removeMember(messageSender);
        humans[humanNum].setGroup(-1);
        cout << "Removed client " << messageSender << " (" << humans[humanNum].getName() << ") from group " << groupNum << endl;
        if (groups[groupNum].getSize() == 1) {
          humanNum = getHumanNum(groups[groupNum].getMember(0));
          humans[humanNum].setGroup(-1);
          cout << "Removed client " << groups[groupNum].getMember(0) << " (" << humans[humanNum].getName() << ") from group " << groupNum << endl;
          groups[groupNum].clear();
          cout << "Cleared group: " << groupNum << endl;
        }
        displayGroups();
      }
      break;*/
      intToChar(messageSender, (unsigned char*) &du[2]);
      // treat a leave as a pop
    case FLAG_GROUP_POP_PARENT:
      // assume a pop could be called when there is no parent group
      clientId = charToInt((unsigned char*) &du[2]);
      humanNum = getHumanNum(clientId);
      if (humanNum > -1 && humans[humanNum].getGroup() > -1) {
        groupNum = humans[humanNum].getGroup();
        groups[groupNum].removeMember(clientId);
        humans[humanNum].setGroup(humans[humanNum].popParentGroup()); // returns -1 if there are no parents
        cout << "Popped client " << clientId << " (" << humans[humanNum].getName() << "), they're now in " << humans[humanNum].getGroup() << endl;
        if (groups[groupNum].getSize() == 1) {
          cout << "Removing remaining user..." << endl;
          // remove the remaining user
          clientId = groups[groupNum].getMember(0);
          humanNum = getHumanNum(clientId);
          // error check?
          if (humanNum > -1) {
            humans[humanNum].setGroup(humans[humanNum].popParentGroup());
            groups[groupNum].clear();
            cout << "Popped remaining human client " << clientId << " (" << humans[humanNum].getName() << ") " << endl;
            cout << "They are now in group " << humans[humanNum].getGroup() << endl;
            cout << "Transmitting this..." << endl;

            string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_POP_PARENT);
            unsigned char c[4];
            intToChar(clientId, c);
            networkData += (char) c[0]; networkData += (char) c[1];
            networkData += (char) c[2]; networkData += (char) c[3];
            for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());
          }else cerr << "Could not pop remaining member - humanNum not found for client " << clientId << endl;
        }//else{ // there's some people in group, but are they all in subgroup?
          // would have to do this recursively
          /*int numInGroup = 0;
          for (int i = 0; i < groups[groupNum].getSize(); i++) {
            if (humans[getHumanNum(groups[groupNum].getMember(i))].getGroup() == groupNum) numInGroup++;
          }
          cout << "numInGroup: " << numInGroup << endl;
          if (numInGroup == 0) { // in subgroup
            for (int i = 0; i < groups[groupNum].getSize(); i++) {
              humanNum = getHumanNum(groups[groupNum].getMember(i));
              if (humanNum > -1) {
                groups[humans[humanNum].getGroup()].removeMember(groups[groupNum].getMember(i));
                humans[humanNum].setGroup(humans[humanNum].popParentGroup());
              }else cerr << "POP_PARENT: human num not found" << endl;
            }
          }*/
        //}
        //displayGroups();
      } else cerr << "Error popping group - humanNum not found for client: " << clientId << ", or they weren't in a group" << endl;
      break;
    case FLAG_GROUP_SET:
      cerr << "Error - received group set flag" << endl;
      break;
    case FLAG_GROUP_UPDATE:
      // client, group, parent1...5
      // bug with this was not remembering integers are four bytes!
      clientId = charToInt((unsigned char*) &du[2]);
      humanNum = getHumanNum(clientId);
      // remove from groups
      for (int g = 0; g < MAX_GROUPS; g++) {
        for (int i = 0; i < groups[g].getSize(); i++) {
          if (groups[g].getMember(i) == clientId) {
            groups[g].removeMember(clientId); // remove's all
            break;
          }
        }
      }

      groupNum = charToInt((unsigned char*) &du[6]);
      //cout << "updating group for client " << clientId << " (" << humans[humanNum].getName() << ") groupNum: " << groupNum << endl;

      humans[humanNum].clearParents(); // still clear even if groupNum is -1 in case joined group in playback

      if (groupNum < MAX_GROUPS) {
        humans[humanNum].setGroup(groupNum);
        if (groupNum > -1) {
          groups[groupNum].addMember(clientId);
          for (int i = 0; i < 5; i++) {
            groupNum = charToInt((unsigned char*) &du[10+i*4]);
            if (groupNum > -1) {
              groups[groupNum].addMember(clientId);
              humans[humanNum].pushParentGroup(groupNum);
            }
          }
          //cout << "group update." << endl;
        }
        
      }else cerr << "group update error: groupNum is more than max groups" << endl;
      //displayGroups();
      break;
    case FLAG_ID:
      /*if (messageSender != SERVER_ID) {
        cerr << "processDataUnit(): ID not assigned - message sender was not server" << endl;
      }else{
        clientId = (int) du[2];
        cerr << "I have been assigned an ID of " << clientId << endl;
        // now we have ID send player name
        //sendPlayerName(); this will be sent upon request when human is made
      }*/
      break;
    case FLAG_LOGOUT:
      clientId = charToInt((unsigned char*) &du[2]);
      // remove client from any groups they are in
      for (int i = 0; i < MAX_GROUPS; i++) {
        groups[i].removeMember(clientId); // removes all instances
        if (groups[i].getSize() == 1) {
          // remove remaining member
          int client = groups[i].getMember(0);
          humanNum2 = getHumanNum(client);
          if (humanNum2 == -1) cerr << "Logout: error finding human num for remaining group member (client: " << client << ")" << endl;
          else humans[humanNum2].setGroup(humans[humanNum2].popParentGroup());
          groups[i].clear();
        }
      }

      humanNum = getHumanNum(clientId);
      if (humanNum == -1) cerr << "Error finding human num for client: " << clientId << " for logout" << endl;
      else {
        cout << "FLAG_LOGOUT received: Logging out " << humans[humanNum].getName() << " (client " << clientId << ", human " << humanNum << ")" << endl;
        humans[humanNum].setActive(false);

        // add on stats
        float addTime = (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0 - statViewpointTime[humanNum].currentStart;
        cout << "adding on viewpoint time: " << addTime << endl;
        if (statViewpointTime[humanNum].currentView == PERSPECTIVE_THIRD_PERSON) statViewpointTime[humanNum].thirdPerson += addTime;
        else if (statViewpointTime[humanNum].currentView == PERSPECTIVE_OVERVIEW) statViewpointTime[humanNum].birdsEye += addTime;
        else cerr << "Current view is not overview or third person for user " << humanNum << ", it's: " << statViewpointTime[humanNum].currentView << endl;

        humanNum2 = statFollowingTime[humanNum].followingHuman;

        if (humanNum2 > -1 && humanNum2 < MAX_HUMANS) {
          if (statFollowedTime[humanNum2].numFollowing > 0) {
            statFollowedTime[humanNum2].numFollowing--;
            if (statFollowedTime[humanNum2].numFollowing == 0) { // no one else following
              cout << "Adding on time at logout" << endl;
              statFollowedTime[humanNum2].cumulative += (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0
                - statFollowedTime[humanNum2].currentStart;
            }
          }else cerr << "LOGOUT Error user not being followed" << endl;
        }

      }
      //displayGroups();
      break;
    case FLAG_NAME_REQUEST:
      //sendPlayerName();
      break;
    case FLAG_NAME:
      if (humanNum > -1 && humanNum < MAX_HUMANS) {
        for (int i = 2; i < length; i++) {
          if (du[i] != ASCII_PAD_DATA) tempStr += du[i];
        }
        //if (humans[humanNum].getName() == "") hudMsg.push_back(tempStr + " is in the game!");
        humans[humanNum].setName(tempStr);
        cout << "Assigned name \"" << tempStr << "\" to human: " << humanNum << endl;
      }else cerr << "Error assigning name, humanNum out of range: " << humanNum << endl;
      break;
    case FLAG_PLAYBACK_FAST:
      if (!playback) {
        cout << "Playback not active. Cannot fast forward." << endl;
      }else{
        if (!playbackFast) {
          cout << "Received playback fast forward signal. Fast forwarding..." << endl;
          playbackFast = true;
        }else{
          cout << "Proceeding with playback" << endl;
          justStartedPlayback = true;
          playbackFast = false;
        }
      }
      break;
    case FLAG_PLAYBACK_PAUSE:
      if (!playback) {
        cout << "Playback not active. Cannot pause." << endl;
      }else{
        playbackFast = false;
        if (!playbackPaused) {
          cout << "Received playback pause signal. Pausing..." << endl;
          playbackPaused = true;
        }else{
          cout << "Proceeding with playback" << endl;
          playbackPaused = false;
          justStartedPlayback = true;
        }
      }
      break;
    case FLAG_PLAYBACK_REWIND:
      if (playbackFile.is_open()) {
        int currentPos = playbackFile.tellg();
        if (currentPos - 20000 > 0) {
          playbackFile.seekg(currentPos - 20000);
          cout << "Rewinding to: " << currentPos - 20000 << endl;
        }else{
          playbackFile.seekg(0);
          cout << "Rewinding to: 0" << endl;
        }
        justStartedPlayback = true; // reset for next time elapsed data
        clearBuffer(serverBufferFile);
        //playback = false;
        playbackPaused = false;
      }
      break;
    case FLAG_PLAYBACK_START:
      playback = true;
      playbackPaused = false;
      playbackFast = false;
      justStartedPlayback = true;
      cout << "Starting playback" << endl;

      if (!playbackFile.is_open()) { // if you have stopped playback, reopen file
        cout << "Reopening file: " << playbackFilename << endl;
        playbackFile.open(playbackFilename.c_str(), ios::in | ios::binary);
        if (!playbackFile.is_open()) { // error check
          cerr << "Error opening file for playback: " << playbackFilename << endl;
          playback = false;
          justStartedPlayback = false;
        }
      }
      break;
    case FLAG_PLAYBACK_STOP:
      cout << "Received playback stop signal. Closing file." << endl;
      playbackFile.close();
      playback = false;
      playbackPaused = false;
      clearBuffer(serverBufferFile);
      break;
    case FLAG_POSITION:
      posX = charToFloat((unsigned char*) &du[2]);
      posY = charToFloat((unsigned char*) &du[6]);
      posZ = charToFloat((unsigned char*) &du[10]);
      angleX = charToFloat((unsigned char*) &du[14]);
      angleY = charToFloat((unsigned char*) &du[18]);
      //cout << "User of ID: " << messageSender << " is at position: "
      //     << charToFloat((unsigned char*) &du[2]) << ","
      //     << charToFloat((unsigned char*) &du[6]) << ","
      //     << charToFloat((unsigned char*) &du[10]) << endl;
      humans[humanNum].setX(posX);
      humans[humanNum].setY(posY);
      humans[humanNum].setZ(posZ);
      humans[humanNum].setAngleX(angleX);
      humans[humanNum].setAngleY(angleY);
      break;
    case FLAG_RECORD_STOP:
      if (recording) {
        cout << "Stopping recording, and closing file." << endl;
        recording = false;
        recordFile.close();
      }else{
        cout << "Got the stop recording signal (from playback file?)" << endl;
      }
      break;
    case FLAG_TALK:
      /*for (int i = 2; i < length; i++) {
        tempStr += du[i];
      }
      tempStr = humans[humanNum].getName() + ": " + tempStr;
      hudMsg.push_back(tempStr);*/
      break;
    case FLAG_TALK_REALTIME:
      break;
    case FLAG_TIME_ELAPSED:
      // if it's more than 60 seconds since the last one then there must be some mistake (due to rewind)
      //cout << charToInt((unsigned char*) &du[2]) - timeElapsed.tv_sec << endl;
      //if (justStartedPlayback || charToInt((unsigned char*) &du[2]) - timeElapsed.tv_sec < 60) {
        timeElapsed.tv_sec = (time_t) charToInt((unsigned char*) &du[2]);
        timeElapsed.tv_usec = (suseconds_t) charToInt((unsigned char*) &du[6]);
        if (justStartedPlayback) {
          gettimeofday(&timeStart, NULL);
          // start straight away
          timeStart.tv_sec -= timeElapsed.tv_sec;
          timeStart.tv_usec -= timeElapsed.tv_usec;
          if (timeStart.tv_usec < 0) {
            // remove a second and add a million
            timeStart.tv_sec--;
            timeStart.tv_usec += 1000000;
          }
          //timeStart.tv_sec -= timeElapsed.tv_sec + 1;
          // add one second, in case timeStart microseconds is about to clock over to next second
          justStartedPlayback = false;
        }
      //}
      break;
    case FLAG_USER_FOLLOW:
      clientId = charToInt((unsigned char*) &du[2]);
      humanNum2 = getHumanNum(clientId);
      if (humanNum > -1 && humanNum < MAX_HUMANS) {
        if (humanNum2 > -1 && humanNum2 < MAX_HUMANS) {

          // if already following them (user clicked again)
          if (humanNum2 == statFollowingTime[humanNum].followingHuman) {
            cout << "User: " << humans[humanNum].getName() << " is still following " << humans[humanNum2].getName() << endl;
          }else{
            cout << "User: " << humans[humanNum].getName() << " is following " << humans[humanNum2].getName() << endl;

            if (statFollowingTime[humanNum].followingHuman > -1) { // if already following someone
              // add on time for that person (as in stopped following them now)
              int h = statFollowingTime[humanNum].followingHuman;

              if (h > -1 && h < MAX_HUMANS) {
                if (statFollowedTime[h].numFollowing > 0) {
                  statFollowedTime[h].numFollowing--;
                  if (statFollowedTime[h].numFollowing == 0) { // no one else following
                    statFollowedTime[h].cumulative += (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0
                      - statFollowedTime[h].currentStart;
                    cout << "...stop following someone first:" << endl;

                    statFollowingTime[humanNum].cumulative +=
                      (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0
                      - statFollowingTime[humanNum].currentStart;

                    cout << "...cumulative following time: " << statFollowingTime[humanNum].cumulative << endl;
                    cout << "...cumulative followed time for " << humans[h].getName() << ": " << statFollowedTime[h].cumulative << endl;
                  }
                }else cerr << "Error... attempt to stop following failed, user " << humans[h].getName() << " not being followed" << endl;
              }else cerr << "Error... user following someone out of range: " << h << endl;
            } // end if following someone already

            statFollowingTime[humanNum].currentStart = (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0;
            statFollowingTime[humanNum].followingHuman = humanNum2;

            // set start times
            if (statFollowedTime[humanNum2].numFollowing == 0) {
              statFollowedTime[humanNum2].currentStart = (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0;
            }
            statFollowedTime[humanNum2].numFollowing++;
            if (statFollowedTime[humanNum2].numFollowing > 1) {
              cout << "More than one person following " << humans[humanNum2].getName() << "!" << endl;
            }

          } // not already following them
        }else cerr << "FLAG_USER_FOLLOW: humanNum2 out of range: " << humanNum2 << " for client: " << clientId << endl;
      }else cerr << "FLAG_USER_FOLLOW: humanNum out of range: " << humanNum << " for client: " << clientId << endl;
      break;
    case FLAG_USER_FOLLOW_STOP:
      if (humanNum > -1 && humanNum < MAX_HUMANS) {

        cout << "User: " << humans[humanNum].getName() << " has stopped following people." << endl;
        statFollowingTime[humanNum].cumulative +=
          (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0
          - statFollowingTime[humanNum].currentStart;

        cout << "...cumulative following time: " << statFollowingTime[humanNum].cumulative << endl;

        // add on time for user being followed
        humanNum2 = statFollowingTime[humanNum].followingHuman;

        statFollowingTime[humanNum].followingHuman = -1;

        if (humanNum2 > -1 && humanNum2 < MAX_HUMANS) {
          if (statFollowedTime[humanNum2].numFollowing > 0) {
            statFollowedTime[humanNum2].numFollowing--;
            if (statFollowedTime[humanNum2].numFollowing == 0) { // no one else following
              statFollowedTime[humanNum2].cumulative += (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0
                - statFollowedTime[humanNum2].currentStart;
              cout << "...cumulative followed time for " << humans[humanNum2].getName() << ": " << statFollowedTime[humanNum2].cumulative << endl;
            }
          }else cerr << "FOLLOW_STOP Error user not being followed" << endl;
        }

      }else cerr << "FLAG_USER_FOLLOW_STOP: humanNum out of range: " << humanNum << " for client: " << messageSender << endl;
      break;
    case FLAG_USER_MEAN:
      if (humanNum > -1 && humanNum < MAX_HUMANS) {
        cout << "User: " << humans[humanNum].getName() << " is moving to the mean location." << endl;
        statMoveToMean[humanNum]++;
      }else cerr << "FLAG_USER_MEAN: humanNum out of range: " << humanNum << " for client: " << messageSender << endl;
      break;
    case FLAG_USER_SCREENSHOT:
      if (humanNum > -1 && humanNum < MAX_HUMANS) {
        cout << "User: " << humans[humanNum].getName() << " has taken a screenshot." << endl;
      }else cerr << "FLAG_USER_SCREENSHOT: humanNum out of range: " << humanNum << " for client: " << messageSender << endl;
      break;
    case FLAG_USER_SELECT: // one user selected another
      clientId = charToInt((unsigned char*) &du[2]);
      humanNum2 = getHumanNum(clientId);
      if (humanNum < 0 || humanNum2 < 0 || humanNum > MAX_HUMANS - 1 || humanNum2 > MAX_HUMANS - 1) {
        cerr << "USER_SELECT: human doesn't exist: 1: " << humanNum << ", 2: " << humanNum2 << endl;
      }else{
        groupNum = humans[humanNum].getGroup();
        // messageSender (humanNum) clicked on clientId (humanNum2)
        if (groupNum == -1) {
          if (humans[humanNum2].getGroup() == -1) newGroup(humans[humanNum], humans[humanNum2], true);
          else {
            // if they've got parents then join parent group
            if (humans[humanNum2].getNumParents() > 0) {
              joinGroup(humans[humanNum2].getParent(0), humans[humanNum]);
            }else joinGroup(humans[humanNum2].getGroup(), humans[humanNum]);
          }
        }else if (humans[humanNum2].getGroup() == groupNum && groups[groupNum].getSize() > 2) {
          // if they're in the same group (not -1) and there's more than two in group then form a subgroup
          // check that there are more than two actually in that group (so can't have whole group in subgroup)
          int numInGroup = 0;
          for (int i = 0; i < groups[groupNum].getSize(); i++) {
            if (humans[getHumanNum(groups[groupNum].getMember(i))].getGroup() == groupNum) numInGroup++;
          }
          cout << "numInGroup: " << numInGroup << endl;
          if (numInGroup > 2) {
            if (humans[humanNum].pushParentGroup(groupNum)) {
              if (humans[humanNum2].pushParentGroup(groupNum)) {
                string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_PUSH_PARENT);
                unsigned char c[4];
                intToChar(humans[humanNum].getClientId(), c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());
                networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_PUSH_PARENT);
                intToChar(humans[humanNum2].getClientId(), c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());

                cout << "Pushed group " << groupNum << " for clients " << humans[humanNum].getClientId() << " and " << humans[humanNum2].getClientId();
                cout << " (" << humans[humanNum].getName() << " and " << humans[humanNum2].getName() << ") - transmitted" << endl;
                newGroup(humans[humanNum], humans[humanNum2], false);
              }else{
                cerr << "Could not subgroup - reached maximum number of parent groups" << endl;
                humans[humanNum].popParentGroup(); // pop the first one back
              }
            }else cerr << "Could not subgroup - reached maximum number of parent groups" << endl;
          }
        }else if (humans[humanNum2].inParent(groupNum) ) {
          // if the one clicking is in the parent group of the other, then push (once max) and join subgroup (if not in parent still)
          // check there is someone left in parent group first
          int numInGroup = 0;
          for (int i = 0; i < groups[groupNum].getSize(); i++) {
            if (humans[getHumanNum(groups[groupNum].getMember(i))].getGroup() == groupNum) numInGroup++;
          }
          cout << "numInGroup: " << numInGroup << endl;
          if (numInGroup > 1) {
            if (humans[humanNum].pushParentGroup(groupNum)) {
              if (humans[humanNum].getGroup() != groupNum) cout << "ERRORR WAAIT A MINUTE" << endl; // TODO can remove this
              string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_PUSH_PARENT);
              unsigned char c[4];
              intToChar(humans[humanNum].getClientId(), c);
              networkData += (char) c[0]; networkData += (char) c[1];
              networkData += (char) c[2]; networkData += (char) c[3];
              for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());
              cout << "Pushed group " << groupNum << " for client " << humans[humanNum].getClientId();
              cout << " (" << humans[humanNum].getName() << ") - transmitted" << endl;

              // if just pushed their last parent then join them (they could be subsubgroup)
              for (int p = 0; p < humans[humanNum2].getNumParents(); p++) {
                if (groupNum == humans[humanNum2].getParent(p)) {
                  if (p == humans[humanNum2].getNumParents() - 1) {
                    joinGroup(humans[humanNum2].getGroup(), humans[humanNum]); // join them
                  }else{
                    joinGroup(humans[humanNum2].getParent(p+1), humans[humanNum]); // join their parent
                  }
                }
              }
            }else cerr << "Could not subgroup - reached maximum number of parent groups" << endl;
          }else{
            // pop the person clicked on
            int groupNum2 = humans[humanNum2].getGroup();
            humans[humanNum2].setGroup(humans[humanNum2].popParentGroup());
            groups[groupNum2].removeMember(clientId);

            string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_POP_PARENT);
            unsigned char c[4];
            intToChar(clientId, c);
            networkData += (char) c[0]; networkData += (char) c[1];
            networkData += (char) c[2]; networkData += (char) c[3];
            for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());

            if (groups[groupNum2].getSize() == 1) {
              cout << "Removing remaining user..." << endl;
              // remove the remaining user
              clientId = groups[groupNum2].getMember(0);
              humanNum = getHumanNum(clientId);
              // error check?
              if (humanNum > -1) {
                humans[humanNum].setGroup(humans[humanNum].popParentGroup());
                groups[groupNum2].clear();
                cout << "Popped remaining human client " << clientId << " (" << humans[humanNum].getName() << ") " << endl;
                cout << "They are now in group " << humans[humanNum].getGroup() << endl;
                cout << "Transmitting this..." << endl;

                networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_POP_PARENT);
                unsigned char c[4];
                intToChar(clientId, c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());
              }else cerr << "Could not pop remaining member - humanNum not found for client " << clientId << endl;
            }
          }
        }else if (humans[humanNum2].getGroup() == -1) {
          // if you're in a group (not -1) but they're not then add them in
          cout << "messageSender " << messageSender << " clicked on " << clientId << " (" << humans[humanNum2].getName() << ") ";
          if (humans[humanNum].getNumParents() > 0) {
            // add them to parent group
            cout << "adding them to parent group" << endl;
            joinGroup(humans[humanNum].getParent(0), humans[humanNum2]);
          }else{
            cout << "who is not in a group - adding them to messageSender's group: " << groupNum << endl;
            joinGroup(groupNum, humans[humanNum2]);
          }
        }else if (humans[humanNum].inParent(humans[humanNum2].getGroup())) {
          // if they're in your parent group then push them and add them
          // but only if they're not last one (don't want everyone in subgroup)
          int groupNum2 = humans[humanNum2].getGroup();
          int numInGroup = 0;
          for (int i = 0; i < groups[groupNum2].getSize(); i++) {
            if (humans[getHumanNum(groups[groupNum2].getMember(i))].getGroup() == groupNum2) numInGroup++;
          }
          cout << "numInGroup: " << numInGroup << endl;

          if (numInGroup > 1) {
            if (humans[humanNum2].pushParentGroup(humans[humanNum2].getGroup())) {
              string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_PUSH_PARENT);
              unsigned char c[4];
              intToChar(humans[humanNum2].getClientId(), c);
              networkData += (char) c[0]; networkData += (char) c[1];
              networkData += (char) c[2]; networkData += (char) c[3];
              for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());
              cout << "messageSender " << messageSender << " (" << humans[humanNum].getName() << ") clicked on client ";
              cout << clientId << " (" << humans[humanNum2].getName() << ")." << endl;
              cout << "  client " << clientId << " is in group " << humans[humanNum2].getGroup() << " which is a parent of messageSender";
              cout << "  so we pushed client " << humans[humanNum2].getClientId() << " (transmitted) ";

              // if just pushed clicker's last parent then join them (they could be subsubgroup)
              for (int p = 0; p < humans[humanNum].getNumParents(); p++) {
                if (groupNum2 == humans[humanNum].getParent(p)) {
                  if (p == humans[humanNum].getNumParents() - 1) {
                    joinGroup(groupNum, humans[humanNum2]); // join them
                  }else{
                    joinGroup(humans[humanNum].getParent(p+1), humans[humanNum2]); // join their parent
                  }
                }
              }
            }else cerr << "Could not subgroup - reached maximum number of parent groups" << endl;
          }else{
            // pop back to parent group so you can speak to them
            humans[humanNum].setGroup(humans[humanNum].popParentGroup());
            groups[groupNum].removeMember(messageSender);

            string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_POP_PARENT);
            unsigned char c[4];
            intToChar(messageSender, c);
            networkData += (char) c[0]; networkData += (char) c[1];
            networkData += (char) c[2]; networkData += (char) c[3];
            for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());

            if (groups[groupNum].getSize() == 1) {
              cout << "Removing remaining user..." << endl;
              // remove the remaining user
              clientId = groups[groupNum].getMember(0);
              humanNum = getHumanNum(clientId);
              // error check?
              if (humanNum > -1) {
                humans[humanNum].setGroup(humans[humanNum].popParentGroup());
                groups[groupNum].clear();
                cout << "Popped remaining human client " << clientId << " (" << humans[humanNum].getName() << ") " << endl;
                cout << "They are now in group " << humans[humanNum].getGroup() << endl;
                cout << "Transmitting this..." << endl;

                networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_POP_PARENT);
                unsigned char c[4];
                intToChar(clientId, c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());
              }else cerr << "Could not pop remaining member - humanNum not found for client " << clientId << endl;
            }
          }
        }else if (humans[humanNum2].getGroup() != groupNum) {
          // if they're in a different group then pop (so that you can join them on next click)
          cout << "popping clicker" << endl;
          humans[humanNum].setGroup(humans[humanNum].popParentGroup());
          groups[groupNum].removeMember(messageSender);

          string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_POP_PARENT);
          unsigned char c[4];
          intToChar(messageSender, c);
          networkData += (char) c[0]; networkData += (char) c[1];
          networkData += (char) c[2]; networkData += (char) c[3];
          for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());

          if (groups[groupNum].getSize() == 1) {
            cout << "Removing remaining user..." << endl;
            // remove the remaining user
            clientId = groups[groupNum].getMember(0);
            humanNum = getHumanNum(clientId);
            // error check?
            if (humanNum > -1) {
              humans[humanNum].setGroup(humans[humanNum].popParentGroup());
              groups[groupNum].clear();
              cout << "Popped remaining human client " << clientId << " (" << humans[humanNum].getName() << ") " << endl;
              cout << "They are now in group " << humans[humanNum].getGroup() << endl;
              cout << "Transmitting this..." << endl;

              networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_POP_PARENT);
              unsigned char c[4];
              intToChar(clientId, c);
              networkData += (char) c[0]; networkData += (char) c[1];
              networkData += (char) c[2]; networkData += (char) c[3];
              for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());
            }else cerr << "Could not pop remaining member - humanNum not found for client " << clientId << endl;
          }
        }
      }
      cout << "end user select" << endl;
      //displayGroups();
      break;
    case FLAG_USER_VIEW:
      if (humanNum > -1 && humanNum < MAX_HUMANS) {
        view = charToInt((unsigned char*) &du[2]);

        float addTime = (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0 - statViewpointTime[humanNum].currentStart;
        if (statViewpointTime[humanNum].currentView == PERSPECTIVE_OVERVIEW) statViewpointTime[humanNum].birdsEye += addTime;
        if (statViewpointTime[humanNum].currentView == PERSPECTIVE_THIRD_PERSON) statViewpointTime[humanNum].thirdPerson += addTime;

        statViewpointTime[humanNum].currentStart = (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0;

          cout << "User " << humans[humanNum].getName() << " is viewing in ";
        if (view == PERSPECTIVE_OVERVIEW) {
          cout << "birds eye view" << endl;
          statViewpointTime[humanNum].currentView = view;
        }else if (view == PERSPECTIVE_THIRD_PERSON) {
          cout << "third person view" << endl;
          statViewpointTime[humanNum].currentView = view;
        }else cout << "an unknown view: " << view << endl;
      }else cerr << "Error: FLAG_USER_VIEW, humanNum out of range: " << humanNum << endl;
      break;
    default:
      cerr << "processDataUnit(): Flag not recognised. integer value: " << (int) flag << endl;
  }
}

int getDataUnitSize(char c)
{
  int size = -1;
  switch (c) {
    case FLAG_AUDIO:
      size = FLAG_AUDIO_SIZE;
      break;
    case FLAG_BULLET:
      size = FLAG_BULLET_SIZE;
      break;
    case FLAG_GROUP_ADD_PARENT:
      size = FLAG_GROUP_ADD_PARENT_SIZE;
      break;
    case FLAG_GROUP_JOIN:
      size = FLAG_GROUP_JOIN_SIZE;
      break;
    case FLAG_GROUP_LEADER:
      size = FLAG_GROUP_LEADER_SIZE;
      break;
    case FLAG_GROUP_LEAVE:
      size = FLAG_GROUP_LEAVE_SIZE;
      break;
    case FLAG_GROUP_NEW:
      size = FLAG_GROUP_NEW_SIZE;
      break;
    case FLAG_GROUP_POP_PARENT:
      size = FLAG_GROUP_POP_PARENT_SIZE;
      break;
    case FLAG_GROUP_PUSH_PARENT:
      size = FLAG_GROUP_PUSH_PARENT_SIZE;
      cerr << "Shouldn't have received FLAG_GROUP_PUSH_PARENT because I send that out!" << endl;
      break;
    case FLAG_GROUP_SET:
      size = FLAG_GROUP_SET_SIZE;
      break;
    case FLAG_GROUP_UPDATE:
      size = FLAG_GROUP_UPDATE_SIZE;
      break;
    case FLAG_ID:
      size = FLAG_ID_SIZE;
      break;
    case FLAG_LOGOUT:
      size = FLAG_LOGOUT_SIZE;
      break;
    case FLAG_NAME:
      size = FLAG_NAME_SIZE;
      break;
    case FLAG_NAME_REQUEST:
      size = FLAG_NAME_REQUEST_SIZE;
      break;
    case FLAG_PLAYBACK_FAST:
      size = FLAG_PLAYBACK_FAST_SIZE;
      break;
    case FLAG_PLAYBACK_PAUSE:
      size = FLAG_PLAYBACK_PAUSE_SIZE;
      break;
    case FLAG_PLAYBACK_REWIND:
      size = FLAG_PLAYBACK_REWIND_SIZE;
      break;
    case FLAG_PLAYBACK_START:
      size = FLAG_PLAYBACK_START_SIZE;
      break;
    case FLAG_PLAYBACK_STOP:
      size = FLAG_PLAYBACK_STOP_SIZE;
      break;
    case FLAG_POSITION:
      size = FLAG_POSITION_SIZE;
      break;
    case FLAG_RECORD_STOP:
      size = FLAG_RECORD_STOP_SIZE;
      break;
    case FLAG_TALK:
      size = FLAG_TALK_SIZE;
      break;
    case FLAG_TALK_REALTIME:
      size = FLAG_TALK_REALTIME_SIZE;
      break;
    case FLAG_TIME_ELAPSED:
      size = FLAG_TIME_ELAPSED_SIZE;
      break;
    case FLAG_USER_FOLLOW:
      size = FLAG_USER_FOLLOW_SIZE;
      break;
    case FLAG_USER_FOLLOW_STOP:
      size = FLAG_USER_FOLLOW_STOP_SIZE;
      break;
    case FLAG_USER_MEAN:
      size = FLAG_USER_MEAN_SIZE;
      break;
    case FLAG_USER_SCREENSHOT:
      size = FLAG_USER_SCREENSHOT_SIZE;
      break;
    case FLAG_USER_SELECT:
      size = FLAG_USER_SELECT_SIZE;
      break;
    case FLAG_USER_VIEW:
      size = FLAG_USER_VIEW_SIZE;
      break;
    default:
      cerr << "getDataUnitSize(): flag not recognised. integer value: " << (int) c << endl;
  }
  return size;
}

// returns whether waitingForMoreData
// sendOut if reading playback data from file... data units
// will be sent out when the time comes for the server to process them
// Other data received will be sent out to all except originating client
bool splitData(Buffer &buf, bool sendOut)
{
  char c;
  int startOfData = -1;
#ifdef _LINUX_
  char removeData[buf.getDataOnBuffer()];
#else
  char removeData[5000];
#endif
  char temp[1];
  int unitSize = 0, bufferSize = 0;
  bool waitingForMoreData = false;
  bool flagNotFound = false;

  struct timeval timeCurrent;
  static bool waitingForTime = false;

  // if fast forwarding then start time should be set to current time minus elapsed time
  // because current - start >= elapsed
  if (playbackFast && !waitingForTime) {
    gettimeofday(&timeStart, NULL);
    timeStart.tv_sec -= timeElapsed.tv_sec;
    timeStart.tv_usec -= timeElapsed.tv_usec;
    if (timeStart.tv_usec < 0) {
      timeStart.tv_sec--;
      timeStart.tv_usec += 1000000;
    }

    // if we get an AUDIO data unit then don't delay afterwards (otherwise audio causes delay
    // in fast forward) - although this doesn't seem to make much difference!
    if (!playbackInstant) {
      // add a slight delay
      timeStart.tv_usec += 1000;
      if (timeStart.tv_usec > 999999) {
        timeStart.tv_sec++;
        timeStart.tv_usec -= 1000000;
      }
    }
    playbackInstant = false;
    waitingForTime = true;
  }
  
  getTimeElapsed(&timeCurrent);
  //if (timeCurrent.tv_sec < timeElapsed.tv_sec - 1) cout << "timeCurrent: " << timeCurrent.tv_sec << ", waiting for timeElapsed: " << timeElapsed.tv_sec << endl;
  //gettimeofday(&timeCurrent, NULL);
  //timeCurrent.tv_sec -= timeStart.tv_sec;
  //timeCurrent.tv_usec -= timeStart.tv_usec;

  //cout << "timeElapsed sec: " << timeElapsed.tv_sec << " timeElapsed usec: " << timeElapsed.tv_usec << endl;
  //cout << "timeCurrent sec: " << timeCurrent.tv_sec << " timeCurrent usec: " << timeCurrent.tv_usec << endl;
  // timeElapsed is set in the processDataUnit which is called within this loop
  while (buf.getDataOnBuffer() > 0 && !waitingForMoreData &&
      (timeCurrent.tv_sec > timeElapsed.tv_sec || (timeCurrent.tv_sec == timeElapsed.tv_sec && timeCurrent.tv_usec >= timeElapsed.tv_usec))) {
    waitingForTime = false;
    // read characters from buffer until we have an ASCII_START_DATA
    startOfData = -1;
    for (int i = 0; i < buf.getDataOnBuffer(); i++) {

      // is the next character the start of data?
      if (!buf.getChar(c, i)) cerr << "splitData(): Error reading from buffer." << endl;
      if (c == ASCII_START_DATA) {
        startOfData = i;
        break;
      }

    }
    
    // if we've not found a data start character then delete the buffer contents
    if (startOfData == -1) {
      cerr << "splitData(): No data start found. Removing buffer contents." << endl;
      bufferSize = buf.getDataOnBuffer();
      if (bufferSize > 0) {
        if (buf.read(removeData, bufferSize) != bufferSize) cerr << "splitData(): Error reading from buffer." << endl;
        cerr << "Buffer size: " << bufferSize << endl;
        /*cerr << "Buffer contents: ";
        for (int i = 0; i < bufferSize; i++) cerr << removeData[i];
        cerr << endl;
        cerr << "Buffer contents (int): ";
        for (int i = 0; i < bufferSize; i++) cerr << (int) removeData[i];
        cerr << endl;
        cerr << "Buffer contents (unsigned int): ";
        for (int i = 0; i < bufferSize; i++) cerr << (unsigned int) removeData[i];
        cerr << endl;*/
      }else{
        cerr << "splitData(): Error. Buffer was empty." << endl;
      }
    }else{

      // remove any data from the beginning of buffer
      if (startOfData > 0) {
        if (buf.read(removeData, startOfData) != startOfData) cerr << "splitData(): Error reading from buffer." << endl;
        else {
          cerr << "splitData(): removing data from beginning of buffer" << endl;
          cerr << " - ASCII_START_DATA was found at: " << startOfData << endl;
          //cerr << " - buffer contents: ";
          //for (int i = 0; i < startOfData; i++) cerr << removeData[i];
          //cerr << endl;
        }
      }

      // we now have a buffer starting with ASCII_START_DATA
      // so check it has a full data unit
      
      // first of all check it has a flag and ID
      if (buf.getDataOnBuffer() > 2) {

        // get size of data unit i.e. data after flag
        if (!buf.getChar(c, 2)) cerr << "splitData(): Error reading flag from buffer." << endl;

        unitSize = getDataUnitSize(c);

        flagNotFound = false;
        if (unitSize < 0) {
          unitSize = 0; // if there's an error just read off the flag and id and don't process
          flagNotFound = true;
        }

        // if there is enough data (total holds unitSize, flag, ID and ASCII_START_DATA)
        if (buf.getDataOnBuffer() > unitSize + 2) {
          // check for ASCII_START_DATA later on within unit, in which case skip current unit
          // but don't do this on integer data values because they can contain ASCII_START_DATA
          // if an integer value is not complete then it'll read it wrong (with part of next data unit)
          // and skip the next data unit
          startOfData = -1;
          //if (c != FLAG_POSITION && c != FLAG_AUDIO) cb
          //if (c == FLAG_AUDIO) cerr << "FLAG_AUDIO RECEIVED" << endl; // TODO I'm sure this isn't necessary (think playback)
          if (c == FLAG_NAME || c == FLAG_NAME_REQUEST_SIZE || c == FLAG_TALK || c == FLAG_TALK_REALTIME) {
            for (int i = 1; i < unitSize + 3; i++) {
              // is the next character the start of data?
              if (!buf.getChar(c, i)) cerr << "splitData(): Error reading from buffer for skip unit check." << endl;
              if (c == ASCII_START_DATA) {
                startOfData = i;
                break;
              }
            }
          }

          if (startOfData > 0) {
            cerr << "splitData(): ASCII_START_DATA found, skipping data unit" << endl;
            if (buf.read(removeData, startOfData) != startOfData) cerr << "splitData(): Error reading from buffer." << endl;
            //cerr << "buffer contents:" << endl;
            //startOfData = buf.getDataOnBuffer();
            // for output :-
            //if (buf.read(removeData, startOfData) != startOfData) cerr << "splitData(): Error reading from buffer." << endl;
            //removeData[startOfData] = 0;
            //cerr << removeData << endl;
            //if (buf.write(removeData) != startOfData) cerr << "splitData(): write error" << endl;
          }else{
            // remove ASCII_START_DATA
            if (buf.read(temp, 1) != 1) cerr << "splitData(): Error reading ASCII_START_DATA from buffer." << endl;

            // create data unit
#ifdef _LINUX_
            char dataUnit[unitSize+2]; // id + flag + data
#else
            char dataUnit[2002]; // 2000 is FLAG_AUDIO_SIZE
#endif
            //cerr << "splitData(): OK. Reading a unit size of " << (unitSize+2) << endl;
            if (buf.read(dataUnit, unitSize+2) != unitSize+2) cerr << "splitData(): Error reading data unit from buffer." << endl;
            // reads unitSize+2 because it reads id, flag, unitSize, no more.

            // process the data!
            if (!flagNotFound) processDataUnit(dataUnit, unitSize+2, sendOut);
          }
        }else{
          // check for ASCII_START_DATA later on, in which case skip current unit
          // some are allowed to contain ASCII_START_CHAR - see comments for similar line above
          startOfData = -1;
          if (c == FLAG_NAME || c == FLAG_NAME_REQUEST_SIZE || c == FLAG_TALK || c == FLAG_TALK_REALTIME) {
            for (int i = 1; i < buf.getDataOnBuffer(); i++) {
              // is the next character the start of data?
              if (!buf.getChar(c, i)) cerr << "splitData(): Error reading from buffer for skip unit check." << endl;
              if (c == ASCII_START_DATA) {
                startOfData = i;
                break;
              }
            }
          }

          if (startOfData > 0) {
            cerr << "splitData(): ASCII_START_DATA found, skipping data unit" << endl;
            if (buf.read(removeData, startOfData) != startOfData) cerr << "splitData(): Error reading from buffer." << endl;
          }else waitingForMoreData = true;
        }

      }else{ // end has flag and ID
        // can't skip if it's an int (but won't skip cos next char is flag?)
        // when buffer had no ASCII_START_DATA and was cleared it just had 1 character which was sometimes
        // the ID of the other client. Other values observed were (ints) -11 and 20
        // that was due to server adding ID after every ASCII_START_DATA
        // now client sends ID instead
        //
        // we have < 3 data elements on the buffer. If second (index 1) is ASCII_START_DATA
        // then skip current unit
        if (buf.getDataOnBuffer() > 1) {
          if (!buf.getChar(c, 1)) cerr << "splitData(): Error reading char from buffer." << endl;
          if (c == ASCII_START_DATA) {
            cerr << "splitData(): Skipping by ONE" << endl;
            if (buf.read(temp, 1) != 1) cerr << "splitData(): Error skipping data unit." << endl;
          }else{
            waitingForMoreData = true;
          }
        }else waitingForMoreData = true;
        
      }

    } // end found data start

    //if (waitingForMoreData)
     //cerr << "splitData(): OK. Waiting for more data..." << endl;

    getTimeElapsed(&timeCurrent);
  } // end while

  return waitingForMoreData;
}

void checkForGroups()
{
  float x1 = 0, x2 = 0, y1 = 0, y2 = 0, z1 = 0, z2 = 0;
  for (int i = 0; i < MAX_HUMANS; i++) {
    // if active and not a member of a group // and not a spectator
    if (humans[i].getActive() && humans[i].getGroup() == -1) {// && humans[i].getClientId() < 50) cb
      x1 = humans[i].getX(), y1 = humans[i].getY(), z1 = humans[i].getZ();

      // check if near any others
      for (int j = 0; j < MAX_HUMANS; j++) {
        if (humans[j].getActive()) { // && humans[j].getClientId() < 50) cb
          x2 = humans[j].getX(), y2 = humans[j].getY(), z2 = humans[j].getZ();

          if (i != j && abs(x2 - x1) < groupRadiusHorizontal && abs(y2 - y1) < groupRadiusVertical && abs(z2 - z1) < groupRadiusHorizontal) {
            // join other's group
            if (humans[j].getGroup() > -1) {
              // error check
              if (humans[j].getGroup() > MAX_GROUPS - 1) {
                cerr << "humans[" << j << "]: group number too high: " << humans[j].getGroup() << endl;
              }else{
                //cout << "human " << i << " has joined " << j << " in group " << humans[j].getGroup() << endl;
                cout << "Check for groups - joining group" << endl;
                if (humans[j].getNumParents() > 0) {
                  joinGroup(humans[j].getParent(0), humans[i]); // join parent
                }else{
                  joinGroup(humans[j].getGroup(), humans[i]); // join them
                }
                /*humans[i].setGroup(humans[j].getGroup());
                groups[humans[j].getGroup()].addMember(humans[i].getClientId());
                cout << "human " << i << " has joined " << j << " in group " << humans[j].getGroup() << endl;
                // send this out
                string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_JOIN);
                unsigned char c[4];
                // groupNum, clientId
                intToChar(humans[j].getGroup(), c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                intToChar(humans[i].getClientId(), c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());*/
              }
            }else{
              // new group
              cout << "Check for groups - new group" << endl;
              newGroup(humans[i], humans[j], true);
              cout << "(humans " << i << " and " << j << ")" << endl;
              /*int nextGroup = getNextGroup();
              if (nextGroup == -1) {
                cerr << "Server::checkForGroup(): Error. No more groups could be allocated" << endl;
              }else{
                // TODO nextGroup doesn't have to be in sequence, as groups have Id's
                // i.e. if groups break up, their array element can be used for the next group
                // at the minute just using element of array as an ID, so could remove group Id
                // selectors and mutators from group.h??
                groups[nextGroup].addMember(humans[i].getClientId());
                groups[nextGroup].addMember(humans[j].getClientId());
                //groups[nextGroup].setLeader(humans[i].getClientId());
                humans[i].setGroup(nextGroup);
                humans[j].setGroup(nextGroup);
                cout << "Created group " << nextGroup << " with humans " << i << " and " << j;
                cout << " (clients " << humans[i].getClientId() << " and " << humans[j].getClientId() << ")" << endl;
                // transmit this group creation across the network
                string networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_NEW);
                unsigned char c[4];
                intToChar(nextGroup, c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                intToChar(humans[i].getClientId(), c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                intToChar(humans[j].getClientId(), c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());*/

                // transmit group leader
                /*networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_LEADER);
                intToChar(nextGroup, c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                intToChar(humans[i].getClientId(), c);
                networkData += (char) c[0]; networkData += (char) c[1];
                networkData += (char) c[2]; networkData += (char) c[3];
                for (int client = 0; client < clCount; client++) sendBuffer[client].write(networkData.c_str(), networkData.length());
                */
              //}
            }
            break; // now part of a group so don't join anyone else
          } // end if near
        } // end if active
      } // end for j

    }
  } // end for i
}

// Do server operations
void doServer()
{
  if (playback && !playbackPaused) {
    // load in a bit at a time. Load next lot when first bit is processed, or we need more data
    if (serverBufferFile.getDataOnBuffer() == 0 || waitingForMoreDataFromFile) {
      //cout << "reading playback file" << endl;
      readPlaybackFile();
    }
    waitingForMoreDataFromFile = splitData(serverBufferFile, true);
  }
  
  receiveData();

  splitData(serverBuffer, false); // deal with data
  checkForGroups();

  sendData();
}

// Accept a new connection and store in client socket array
void acceptConnection()
{
  sin_size = sizeof(struct sockaddr_in);

  int check;

#ifdef _LINUX_
  if ((check = accept(listenSock, (struct sockaddr *)&their_addr, (socklen_t*) &sin_size)) == -1) {
#else
  if ((check = accept(listenSock, (struct sockaddr *)&their_addr, (int*) &sin_size)) == -1) {
#endif
    cerr << "Server::acceptConnection(): accept error" << endl;
  }else{
    cerr << "Server::acceptConnection(): OK. Got connection from " << inet_ntoa(their_addr.sin_addr) << endl;
    if (clCount < MAX_CLIENTS){ // don't allow infinite connections!
      clSocks[clCount] = check;

      // make ID
      id[clCount] = getNextId();
      cout << "Assigning ID: " << (int) id[clCount] << " --------------" << endl;

      // add address to UDP list
      clSocksUdp[clCount] = their_addr;
      clSocksUdp[clCount].sin_port = htons(PORT + 2);

      // the connection has been accepted and so give them their id
      char idData[4];
      idData[0] = ASCII_START_DATA;
      idData[1] = SERVER_ID;
      idData[2] = FLAG_ID;
      idData[3] = id[clCount];
      
      if (sendBuffer[clCount].write(idData, 4) != 4) {
        cerr << "Server::acceptConnection(): Error when attempting to write id info to buffer " << clCount << endl;
      }
      
      // send names of others
      string networkData = "", tempName = "";
      unsigned char c[4];
      int humanNum = 0;

      for (int i = 0; i < clCount; i++) {
        networkData = makeNetworkData(id[i], FLAG_NAME);
        humanNum = getHumanNum(id[i]);
        if (humans[humanNum].getName() != "") {
          tempName = humans[humanNum].getName();
          while (tempName.length() < FLAG_NAME_SIZE) { tempName += ASCII_PAD_DATA; }
          networkData += tempName;
          if (sendBuffer[clCount].write(networkData.c_str(), networkData.length()) != (int) networkData.length()) {
            cerr << "Server::acceptConnection(): Error when attempting to write name info to buffer " << clCount << endl;
          }
        }
      }

      // if playing back then send recorded names too
      if (playback) {

        bool found = false;

        for (int i = 0; i < MAX_HUMANS; i++) {
          if (humans[i].getActive()) {

            // check it's not a current client
            found = false;
            for (int j = 0; j < clCount; j++) {
              if (humans[i].getClientId() == id[j]) found = true;
            }
            if (!found && humans[i].getName() != "") {
              networkData = makeNetworkData(humans[i].getClientId(), FLAG_NAME);
              tempName = humans[i].getName();
              // TODO check name is not too long? and above?
              while (tempName.length() < FLAG_NAME_SIZE) { tempName += ASCII_PAD_DATA; }
              networkData += tempName;
              cout << "Sending name " << humans[i].getName() << " to client " << id[clCount] << endl;
              if (sendBuffer[clCount].write(networkData.c_str(), networkData.length()) != (int) networkData.length()) {
                cerr << "Server::acceptConnection(): Error when attempting to write recorded name info to buffer " << clCount << endl;
              }
            }

          }
        }

      }

      // send group info
      //
      // firstly groups array
      int groupSize = 0;
      for (int i = 0; i < MAX_GROUPS; i++) {
        groupSize = groups[i].getSize();
        if (groupSize == 1) {
          cerr << "Server::acceptConnection(): Error. Group size of 1, for group " << i << endl;
          cerr << "Sending out this group info anyway" << endl;
          networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_UPDATE);
          intToChar(groups[i].getMember(0), c);
          networkData += (char) c[0]; networkData += (char) c[1];
          networkData += (char) c[2]; networkData += (char) c[3];
          intToChar(i, c);
          networkData += (char) c[0]; networkData += (char) c[1];
          networkData += (char) c[2]; networkData += (char) c[3];
          int n = getHumanNum(groups[i].getMember(0));
          for (int p = 0; p < 5; p++) {
            if (n > -1 && p < humans[n].getNumParents()) {
              intToChar(humans[n].getParent(p), c);
              networkData += c[0]; networkData += c[1];
              networkData += c[2]; networkData += c[3];
            }else{
              intToChar(-1, c);
              networkData += c[0]; networkData += c[1];
              networkData += c[2]; networkData += c[3];
            }
          }
          if (sendBuffer[clCount].write(networkData.c_str(), networkData.length()) != (int) networkData.length()) {
            cerr << "Server::acceptConnection(): Error when attempting to write new group info to buffer " << clCount << endl;
          }
        }
        if (groupSize > 1) {
          // create a group with first two
          networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_NEW);
          // using array element as group id
          intToChar(i, c);
          networkData += (char) c[0]; networkData += (char) c[1];
          networkData += (char) c[2]; networkData += (char) c[3];
          intToChar(groups[i].getMember(0), c);
          networkData += (char) c[0]; networkData += (char) c[1];
          networkData += (char) c[2]; networkData += (char) c[3];
          intToChar(groups[i].getMember(1), c);
          networkData += (char) c[0]; networkData += (char) c[1];
          networkData += (char) c[2]; networkData += (char) c[3];

          if (sendBuffer[clCount].write(networkData.c_str(), networkData.length()) != (int) networkData.length()) {
            cerr << "Server::acceptConnection(): Error when attempting to write new group info to buffer " << clCount << endl;
          }

          // add the rest of the group
          for (int j = 2; j < groupSize; j++) {
            networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_JOIN);
            // groupNum, clientId
            intToChar(i, c);
            networkData += (char) c[0]; networkData += (char) c[1];
            networkData += (char) c[2]; networkData += (char) c[3];
            intToChar(groups[i].getMember(j), c);
            networkData += (char) c[0]; networkData += (char) c[1];
            networkData += (char) c[2]; networkData += (char) c[3];
            if (sendBuffer[clCount].write(networkData.c_str(), networkData.length()) != (int) networkData.length()) {
              cerr << "Server::acceptConnection(): Error when attempting to write group join info to buffer " << clCount << endl;
            }
          }
        }
      }

      // then send human parent info
      for (int i = 0; i < MAX_HUMANS; i++) {
        if (humans[i].getActive()) {
          for (int j = 0; j < humans[i].getNumParents(); j++) {
            cout << "Sending to new client. Client " << humans[i].getClientId() << " has parent " << humans[i].getParent(j) << endl;
            networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_ADD_PARENT);
            // clientId, parent
            intToChar(humans[i].getClientId(), c);
            networkData += (char) c[0]; networkData += (char) c[1];
            networkData += (char) c[2]; networkData += (char) c[3];
            intToChar(humans[i].getParent(j), c);
            networkData += (char) c[0]; networkData += (char) c[1];
            networkData += (char) c[2]; networkData += (char) c[3];
            if (sendBuffer[clCount].write(networkData.c_str(), networkData.length()) != (int) networkData.length()) {
              cerr << "Server::acceptConnection(): Error when attempting to write parent info to buffer " << clCount << endl;
            }
          }
          cout << "Sending to new client. Client " << humans[i].getClientId() << " is in group " << humans[i].getGroup() << endl;
          networkData = makeNetworkData(SERVER_ID, FLAG_GROUP_SET);
          // clientId, groupNum
          intToChar(humans[i].getClientId(), c);
          networkData += (char) c[0]; networkData += (char) c[1];
          networkData += (char) c[2]; networkData += (char) c[3];
          intToChar(humans[i].getGroup(), c);
          networkData += (char) c[0]; networkData += (char) c[1];
          networkData += (char) c[2]; networkData += (char) c[3];
          if (sendBuffer[clCount].write(networkData.c_str(), networkData.length()) != (int) networkData.length()) {
            cerr << "Server::acceptConnection(): Error when attempting to write group set info to buffer " << clCount << endl;
          }
        }
      }

      clCount++; // one more client!

    }else{
      cerr << "Error. Reached maximum number of clients. Closing socket." << endl;
#ifdef _LINUX_
      if (close(check) == -1)
#else
      if (closesocket(check) == -1)
#endif
      {
        cerr << "Server::accept(): close error" << endl;
        perror("perror close()");
      }
    }
  }
  
}

void recordStart(const char* optarg)
{
  playbackFile.open(optarg, ios::in | ios::binary);
  if (playbackFile.is_open()) {
    cout << "File exists: " << optarg << endl;
    cout << "Proceed with overwrite (y/n)? ";
    string str;
    cin >> str;
    if (str != "y") {
      cout << "Overwrite cancelled. Exiting." << endl;
      exit(0);
    }
  }
  playbackFile.close();
      
  recordFile.open(optarg, ios::out | ios::binary);
  if (!recordFile.is_open()) {
    cerr << "Error opening file for recording. Exiting." << endl;
    exit(1);
  }else{
    cout << "Recording to file " << optarg << endl;
    recording = true;
  }
}

void playbackInit(char* optarg)
{
  playbackFilename = optarg;

  playbackFile.open(optarg, ios::in | ios::binary | ios::ate);

  if (!playbackFile.is_open()) {
    cerr << "Error opening file for playback. Exiting." << endl;
    exit(1);
  }else{
    cout << "Will play back from file " << optarg << " when signal is received" << endl;
    filesize = playbackFile.tellg();
    cout << "Filesize: " << filesize << endl;
    playbackFile.seekg(0, ios::beg);
    nextId = 50; // so spectator's ID's don't interfere with recorded ID's
  }

}

void finish(int c)
{
  cout << "Signal: " << c << " received" << endl;
  if (playbackFile.is_open()) {
    cout << "Closing playback file." << endl;
    playbackFile.close();
  }
  if (recordFile.is_open()) {
    cout << "Closing record file." << endl;
    recordFile.close();
  }
  // output stats
  cout << "Begin stat output. Amount of time spent following someone:" << endl;
  for (int i = 0; i < MAX_HUMANS; i++) {
    cout << i << " (" << humans[i].getName() << "): " << statFollowingTime[i].cumulative << endl;
  }
  cout << "Amount of time being followed:" << endl;
  for (int i = 0; i < MAX_HUMANS; i++) {
    cout << i << " (" << humans[i].getName() << "): " << statFollowedTime[i].cumulative << " -- " << statFollowedTime[i].cumulative / 60.0 << " mins" << endl;
  }
  cout << "Number of times moved to mean location of group:" << endl;
  for (int i = 0; i < MAX_HUMANS; i++) {
    cout << i << " (" << humans[i].getName() << "): " << statMoveToMean[i] << endl;
  }
  cout << "Amount of time spent in each viewpoint:" << endl;

  float addTime = 0.0;
  for (int i = 0; i < MAX_HUMANS; i++) {
    if (humans[i].getActive()) {
      cout << "human " << i << " (" << humans[i].getName() << ") active" << endl;
      // add on latest time (since it only accumulates when viewpoint is changed)
      // If they've logged out this will already have been done
      addTime = (float) timeElapsed.tv_sec + (float) timeElapsed.tv_usec / 1000000.0 - statViewpointTime[i].currentStart;
      if (statViewpointTime[i].currentView == PERSPECTIVE_THIRD_PERSON) statViewpointTime[i].thirdPerson += addTime;
      else if (statViewpointTime[i].currentView == PERSPECTIVE_OVERVIEW) statViewpointTime[i].birdsEye += addTime;
      else cerr << "Current view is not overview or third person for user " << i << ", it's: " << statViewpointTime[i].currentView << endl;
    }else 
      cout << "human " << i << " (" << humans[i].getName() << ") inactive" << endl;

      cout << i << " (" << humans[i].getName() << ") Birdseye: " << statViewpointTime[i].birdsEye << ", Third Person: " << statViewpointTime[i].thirdPerson << endl;
  }

  cout << "Exiting..." << endl;
  exit(0);
}

// The main function
//   argc - the number of command line arguments
//   argv - the command line arguments
//
// Returns:
//   zero as required by standards
int main(int argc, char** argv)
{
  signal(SIGINT, finish);
  
  serverInit();

#ifdef _LINUX_
  // process CLAs
  struct option options[] = {
    {"help", no_argument, 0, 'h'},
    {"record", required_argument, 0, 'r'},
    {"playback", required_argument, 0, 'p'},
    {0, 0, 0, 0}
  };
  
  int optionIndex = 0; // stores which element of the options array has been found
  int c = 0; // the return value for getopt_long

  while ((c = getopt_long(argc, argv, "hr:p:", options, &optionIndex)) != EOF) {
    switch (c) {
      case 'h':
        cout << "Usage: ./server [-h|--help] [-r|--record <file> or -p|--playback <file>]" << endl;
        cout << "  -r, --record: record all received data to <file>" << endl;
        cout << "  -p, --playback: playback from <file>" << endl;
        exit(0);
        break;
      case 'r':
        recordStart(optarg);
        break;
      case 'p':
        playbackInit(optarg);
        break;
    }
  }
#else
  if (argc > 1) {
    string str = argv[1];
    if (str == "-h" || str == "--help" || str == "/?" || str == "/h" || str == "/help" || str == "?") {
      cout << "Usage: ./server [-h|--help] [-r|--record <file> or -p|--playback <file>]" << endl;
      cout << "  -r, --record: record all received data to <file>" << endl;
      cout << "  -p, --playback: playback from <file>" << endl;
      exit(0);
    }
    if (argc > 2) { // program name, option and filename
      if (str == "-r" || str == "--record") {
        recordStart(argv[2]);
      }else if (str == "-p" || str == "--playback") {
        playbackInit(argv[2]);
      }
    }
  }
#endif
  
  int numReadable = 0;
  
  while (1) { // keep listening and serving
    //timeout.tv_sec = 0; // not used (timeout set to NULL in select so if sockets are blocked then doesn't waste CPU cycles)
    //timeout.tv_usec = 0;

    FD_ZERO(&readSocks);
    FD_ZERO(&writeSocks);
    FD_SET(listenSock, &readSocks); // set up the listening sock
    FD_SET(udpSock, &readSocks);
    FD_SET(udpSock, &writeSocks);
    maxSock = listenSock; // these are the only sockets to begin with
    // so one must be the maximum one
    if (udpSock > maxSock) maxSock = udpSock;

    for (int i = 0; i < clCount; i++) {
      FD_SET(clSocks[i], &readSocks);
      FD_SET(clSocks[i], &writeSocks);
      // if they've closed a connection, maxSock might decrease
      if (clSocks[i] > maxSock) maxSock = clSocks[i];
    }
  
    //if ((numReadable = select(maxSock + 1, &readSocks, &writeSocks, NULL, &timeout)) == -1) curly bracket
    // don't waste processing time and halt here if serverBuffer is empty
    //cout << "Server buffer size: " << serverBuffer.getDataOnBuffer() << endl;
    // server buffer will be empty because it is processed after receive. It may exit splitData waiting for more
    // data (i.e. not empty) but this will mean it has to wait to receive data from the socket anyway
    //if ((numReadable = select(maxSock + 1, &readSocks, &writeSocks, NULL, (serverBuffer.getDataOnBuffer() > 0) ? &timeout : NULL)) == -1) cb
    if ((numReadable = select(maxSock + 1, &readSocks, &writeSocks, NULL, NULL)) == -1){
      cerr << "Server::main(): select error" << endl;
      perror("perror");
      continue; // error with select so ignore the rest of loop
    }

    // if the listenSock has been set as readable, then accept the incoming connection
    if (FD_ISSET(listenSock, &readSocks)) acceptConnection();

    // main function call for server
    // sends and receives data
    // if there are no sockets readable then there is no need to try to send
    // and receive data because the readSocks and writeSocks checks will fail
    // however doServer() contains other server processing info
    doServer();

    // analyse traffic
    //if (clock() > trafficAnalysisTimer) analyseTraffic();
  }

  return 0;
}

