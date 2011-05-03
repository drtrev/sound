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
 * client.h
 *
 * A client object, designed to deal with the networking side of the CVE.
 * This client implements socket programming which was inspired by a
 * combination of the example code on Beej's Guide to Network Programming
 * http://www.ecst.csuchico.edu/~beej/guide/net/ (last accessed 15/11/2004)
 * and the appendix of Singhal, S. & Zyda, M. (1999) Networked Virtual Environments:
 * Design and Implementation. Reading, MA ; Harlow, England : Addison-Wesley
 *
 * Trevor Dodds, 2005
 */

#ifndef _CLIENT_
#define _CLIENT_

#include <errno.h>

#ifdef _LINUX_

#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h> // for gettimeofday tests
#include <netinet/in.h>
#include <sys/socket.h>

#else

#include <winsock.h>

#endif

#include <vector>
#include "network.h"
#include "buffer.h"

using namespace std;

class Client {

  private:
    struct hostent *host;
    struct sockaddr_in serverAddress; // server address information 
    struct sockaddr_in serverAddressUdp; // server address information 
    struct sockaddr_in my_addr_udp;    // my address information
    struct timeval timeout; // timeout for using select()
    fd_set readSocks; // sockets to read from
    fd_set writeSocks; // sockets to write to
    int sockfd; // socket file descriptor
    int sockfdUdp; // udp socket
    int readyToSend;
    Buffer sendBuffer, sendBufferUdp;
    int id;
    bool connected;
#ifndef _LINUX_
    bool winSockInit;
    WSADATA wsaData;
#endif

    //struct timeval timeReceive;

    void transmitBufferedData();
    int receiveData(char[]);
    int receiveDataUdp(char[]);

  public:
    // Constructor
    Client();

#ifndef _LINUX_
    void initWinSock();
#endif

    // Initialise connection
    bool connectToServer(const char*);

    // Allocate data to be sent in the next call to doClient()
    void sendData(const char*, const int);
    void sendDataUdp(const char*, const int);

    // Send and receive data
    bool doClient(char[], int&, char[], int&);

    // Selector for connection status
    bool getConnected();

    void clearBuffers();

    // Close connection to server
    int closeConnection();

};

#endif
