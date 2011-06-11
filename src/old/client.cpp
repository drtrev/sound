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

#include "client.h"

// The client constructor
Client::Client()
{
  id = -1;
  connected = false;
#ifndef _LINUX_
  winSockInit = false;
#endif
}

// Initialise the winsock environment
#ifndef _LINUX_
void Client::initWinSock()
{
  if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
    fprintf(stderr, "WSAStartup failed.\n");
    exit(1);
  }
  else
  {
    cout << "WSAStartup successful" << endl;
    winSockInit = true;
  }
}
#endif

// Initialise server address struct and attempt connection to server
bool Client::connectToServer(const char* ip)
{
  if (connected) {
    cerr << "Client::connect(): error trying to connect." << endl
         << "  - 'connected' is true indicating a connection should already be present" << endl;
    return false; // stop and return an error
  }

#ifndef _LINUX_
  if (!winSockInit) initWinSock();
#endif
  
  host = gethostbyname(ip);
  if (h_errno == HOST_NOT_FOUND) {
    cerr << "Client::connect(): host not found" << endl;
#ifdef _LINUX_
    herror("gethostbyname herror"); // herror instead of perror
#endif
    return false;
  }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    cerr << "Client::connectToServer(): error connecting to server. socket() returned an error" << endl;
    perror("perror socket()");
    return false; // stop and return an error
  }
  if ((sockfdUdp = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    cerr << "Client::connectToServer(): error connecting to server using UDP" << endl;
    perror("perror socket()");
    return false; // stop and return an error
  }

  serverAddress.sin_family = AF_INET;    // host byte order 
  serverAddress.sin_port = htons(PORT);  // short, network byte order 
  serverAddress.sin_addr = *((struct in_addr *)host->h_addr);
  memset(&(serverAddress.sin_zero), '\0', 8);  // zero the rest of the struct 

  serverAddressUdp.sin_family = AF_INET;    // host byte order 
  serverAddressUdp.sin_port = htons(PORT+1);  // short, network byte order 
  serverAddressUdp.sin_addr = *((struct in_addr *)host->h_addr);
  memset(&(serverAddressUdp.sin_zero), '\0', 8);  // zero the rest of the struct 

  my_addr_udp.sin_family = AF_INET;         // host byte order
  my_addr_udp.sin_port = htons(PORT+2);       // short, network byte order
  my_addr_udp.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr_udp.sin_zero), '\0', 8); // zero the rest of the struct

  if (connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr)) == -1) {
    cerr << "Client::connectToServer(): error connecting to server. connect() returned an error" << endl;
    perror("perror connect()");
    return false; // error
  }else
    connected = true;

  if (bind(sockfdUdp, (struct sockaddr *)&my_addr_udp, sizeof(struct sockaddr)) == -1) {
    cerr << "Client::connectToServer(): error setting up udpSock with bind()" << endl;
    perror("bind");
    //exit(1);
  }

  /*char buffer[5];
  buffer[0] = 'a';
  buffer[1] = 'a';
  buffer[2] = '\0';
  buffer[3] = 'a';
  buffer[4] = 'a';

  if (send(sockfd, buffer, 5, 0) == -1) {
    cerr << "Client::connectToServer(): error with send()" << endl;
    perror("perror send()");
  }*/

  cout << "Connected to server " << ip << endl;
  return true; // success
}

void Client::sendData(const char* data, const int length)
{
  // sendData actually buffers data to send.
  // doClient should be called every loop,
  // and this transmits any buffered data assuming the socket is writable.

  int bytesStored = sendBuffer.write(data, length);
  //cerr << "client::sendData(): stored " << bytesStored << " on buffer" << endl;
  if (bytesStored != length) {
    cerr << "Client::sendData(): send buffer overflowed. Some data will not be sent" << endl;
    int bufferSize = sendBuffer.getDataOnBuffer();
#ifdef _LINUX_
    char temp[bufferSize+1];
#else
    char temp[5000];
#endif
    sendBuffer.read(temp, bufferSize);
    temp[bufferSize] = 0; // null character to end string for output
    cerr << "  - buffer contents: " << temp << endl;
    sendBuffer.write(temp, bufferSize); // we've just removed the data from the buffer for output
  }
}

void Client::sendDataUdp(const char* data, const int length)
{
  int bytesStored = sendBufferUdp.write(data, length);
  //cerr << "client::sendData(): stored " << bytesStored << " on buffer" << endl;
  if (bytesStored != length) {
    cerr << "Client::sendDataUdp(): send buffer overflowed. Some data will not be sent" << endl;
    int bufferSize = sendBufferUdp.getDataOnBuffer();
#ifdef _LINUX_
    char temp[bufferSize+1];
#else
    char temp[10000];
#endif
    sendBufferUdp.read(temp, bufferSize);
    temp[bufferSize] = 0; // null character to end string for output
    cerr << "  - buffer contents: " << temp << endl;
    sendBufferUdp.write(temp, bufferSize); // we've just removed the data from the buffer for output
  }
}

// Transmit data that has been buffered for sending
void Client::transmitBufferedData()
{
  // get amount of data on the buffer
  int amountToRead = sendBuffer.getDataOnBuffer();

  // if there is data on the buffer
  if (amountToRead > 0) {
  
    // we've got a writeable socket!
    if (FD_ISSET(sockfd, &writeSocks)) {
      
      // read the data from buffer
#ifdef _LINUX_
      char buffer[amountToRead];
#else
      char buffer[5000];
#endif
      int amountOfDataRead = sendBuffer.read(buffer, amountToRead);
      
      if (amountOfDataRead != amountToRead) {
        cerr << "Client::transmitBufferedData(): error when reading from buffer." << endl;
        cerr << "  - amountOfDataRead: " << amountOfDataRead << ", amountToRead: " << amountToRead << endl;
      }else{
        // send with error check
        // TODO - this doesn't necessarily send all of buffer, see sendall() on Beej's tutorial
        // TODO - does recv have the same problem??
        // note that this returns the number of bytes sent, so that can be used to send the rest later
        if (send(sockfd, buffer, amountOfDataRead, 0) == -1) {
          cerr << "Client::transmitBufferedData(): error with send()" << endl;
          perror("perror send()");
        }
      }
      
    } // end if socket is writable

  } // end if there is data on the buffer

  // do the same for UDP connection
  
  amountToRead = sendBufferUdp.getDataOnBuffer();
  if (amountToRead > 0) {
    if (FD_ISSET(sockfdUdp, &writeSocks)) {
#ifdef _LINUX_
      char buffer[amountToRead];
#else
      char buffer[10000];
#endif
      int amountOfDataRead = sendBufferUdp.read(buffer, amountToRead);
      
      if (amountOfDataRead != amountToRead) {
        cerr << "Client::transmitBufferedData(): error when reading from buffer." << endl;
        cerr << "  - amountOfDataRead: " << amountOfDataRead << ", amountToRead: " << amountToRead << endl;
      }else{
        // send with error check
        if (sendto(sockfdUdp, buffer, amountOfDataRead, 0, (struct sockaddr *) &serverAddressUdp, sizeof(struct sockaddr)) == -1) {
          cerr << "Client::transmitBufferedData(): error with send()" << endl;
          perror("perror send()");
        }
        //cout << "Sent UDP data" << endl;
      }
    } // end if socket is writable
  }
}

int Client::receiveData(char buf[])
{
  int bytesReceived = 0;

  // if the socket is readable
  if (FD_ISSET(sockfd, &readSocks)) {
      
    if ((bytesReceived = recv(sockfd, buf, MAX_RECV_DATA, 0)) == -1) {
      int errorNum = errno; // preserve before other library calls
      cerr << "Client::receiveData(): error with recv()" << endl;
      perror("perror recv()");
      cerr << "checking to see if this is a connection reset" << endl;

#ifdef _LINUX_
      if (errorNum == ECONNRESET) {
#else
      if (errorNum == WSAECONNRESET) {
#endif
        cerr << "yes it is. closing socket. return value: " << closeConnection() << endl;
      }
    }

    if (bytesReceived == 0) {
      cerr << "Client::receiveData(): OK. Server has closed the connection" << endl;
      cerr << "  - Closing. Return value: " << closeConnection() << endl;
      // closeConnection() sets connected to false
    }else{
      //buf[bytesReceived] = 0; // set the element after last to null
      //cout << "Bytes received: " << bytesReceived << endl;
      //cout << "Strlen(buf): " << strlen(buf) << endl;
      //cout << "Data: " << buf << endl;
    }

  } // end if socket is readable (if it's not then there's no data to be read)

  return bytesReceived;
}

int Client::receiveDataUdp(char buf[])
{
  int bytesReceived = 0;

  // if the socket is readable
  if (FD_ISSET(sockfdUdp, &readSocks)) {
      
#ifdef _LINUX_
    socklen_t addr_len = sizeof(struct sockaddr);
#else
	int addr_len = sizeof(struct sockaddr);
#endif
    // should always be receiving from server so could us a seperate struct here just in case
    // a fake connection overwrites server details
    // serverAddressUdp not used here because that is correct and this is filled in
    if ((bytesReceived = recvfrom(sockfdUdp, buf, MAX_RECV_DATA, 0, (struct sockaddr *) &serverAddress, &addr_len)) == -1) {
      cerr << "Client::receiveData(): error with recv()" << endl;
      perror("perror recv()");
    }

    if (bytesReceived == 0) {
      cerr << "Client::receiveDataUdp(): No bytes received" << endl;
    }

    //gettimeofday(&timeReceive, NULL);
    //cout << "Received data at: " << timeReceive.tv_sec << ":" << timeReceive.tv_usec << " bytes: " << bytesReceived << " (before processing)" << endl;

  } // end if socket is readable (if it's not then there's no data to be read)
  else{
    //cout << "UDP socket blocked" << endl;
  }

  return bytesReceived;

}

// sends data and then receives data into buf
// buf should be of size MAX_RECV_DATA
bool Client::doClient(char buf[], int &len, char bufUdp[], int &lenUdp)
{
  if (!connected) return false; // error

  // check the socket to see if it is writable or readable using select()
  int numReadable = 0;

  // zero time out - don't sit there waiting we're working in real time!
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  FD_ZERO(&readSocks);
  FD_ZERO(&writeSocks);
  FD_SET(sockfd, &readSocks); // check for sockfd being readable
  FD_SET(sockfd, &writeSocks); // check for sockfd being writable
  FD_SET(sockfdUdp, &readSocks);
  FD_SET(sockfdUdp, &writeSocks);
  int maxSock = sockfd;
  if (sockfdUdp > maxSock) maxSock = sockfdUdp;

  // do the select() call
  if ((numReadable = select(maxSock + 1, &readSocks, &writeSocks, NULL, &timeout)) == -1) {
    cerr << "Client::doClient(): select() error" << endl;
    perror("perror select()");
  }

  // receive data from socket
  // this may close connection if socket is readable and bytes received is zero
  lenUdp = receiveDataUdp(bufUdp);
  len = receiveData(buf);
  if (!connected) {
    cerr << "Client::doClient(): OK we have a closed connection now from receiveData()." << endl;
  }else{

    // transmit buffered data
    transmitBufferedData();

    // do server operations here

  }
  
  return connected;
}

// Returns the connection status
bool Client::getConnected()
{
  return connected;
}

// clear anything that was going to be sent
void Client::clearBuffers()
{
  int bufferSize = sendBuffer.getDataOnBuffer();
  char removeData[BUFFER_SIZE]; // TODO this is just bad because buffer size may change

  if (bufferSize > 0) {
    if (sendBuffer.read(removeData, bufferSize) != bufferSize) cerr << "Client::clearBuffers(): Error reading from buffer." << endl;
  }
  bufferSize = sendBufferUdp.getDataOnBuffer();
  if (bufferSize > 0) {
    if (sendBufferUdp.read(removeData, bufferSize) != bufferSize) cerr << "Client::clearBuffers(): Error reading from UDP buffer." << endl;
  }
}

// Close the connection with the server
//
// Returns:
//   int - return value from connection close operation
int Client::closeConnection()
{
  cerr << "Client::closeConnection(): OK. Closing connection to server." << endl;
  connected = false;
#ifdef _LINUX_
  cout << "UDP: " << close(sockfdUdp) << endl;
  return close(sockfd);
#else
  cout << "UDP: " << closesocket(sockfdUdp) << endl;
  return closesocket(sockfd);
#endif
}
