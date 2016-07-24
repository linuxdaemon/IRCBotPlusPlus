/*
 *   C++ sockets on Unix and Windows
 *   Copyright (C) 2002
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PracticalSocket.h"

#ifdef WIN32
#include <winsock.h>         // For socket(), connect(), send(), and recv()
typedef int socklen_t;
typedef char raw_type;       // Type used for raw data on this platform
#else

#include <sys/socket.h>      // For socket(), connect(), send(), and recv()
#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()

typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <cstring>
#include <sstream>
#include <iostream>
#include <cassert>

using namespace std;

#ifdef WIN32
static bool initialized = false;
#endif

// SocketException Code

SocketException::SocketException(const string &message, bool inclSysMsg)
throw() : userMessage(message) {
    if (inclSysMsg) {
        userMessage.append(": ");
        userMessage.append(strerror(errno));
    }
}

SocketException::~SocketException() throw() {
}

const char *SocketException::what() const throw() {
    return userMessage.c_str();
}

// Function to fill in address structure given an address and port
static void fillAddr(const string &address, unsigned short port, addrinfo **addr) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;
    stringstream p;
    p << port;
    int rv;
    if ((rv = getaddrinfo(address.c_str(), p.str().c_str(), &hints, addr)) != 0) {
        cout << gai_strerror(rv) << endl;
        throw SocketException("Failed to resolve name (gethostbyname())");
    }
    cout << "Address filled" << endl;
    //freeaddrinfo(&hints);
}

// Socket Code

Socket::Socket(int type, int protocol) throw(SocketException) {
#ifdef WIN32
    if (!initialized) {
      WORD wVersionRequested;
      WSADATA wsaData;

      wVersionRequested = MAKEWORD(2, 0);              // Request WinSock v2.0
      if (WSAStartup(wVersionRequested, &wsaData) != 0) {  // Load WinSock DLL
        throw SocketException("Unable to load WinSock DLL");
      }
      initialized = true;
    }
#endif

    // Make a new socket
    if ((sockDesc = socket(PF_INET, type, protocol)) < 0) {
        throw SocketException("Socket creation failed (socket())", true);
    }
}

Socket::Socket(int sockDesc) {
    this->sockDesc = sockDesc;
}

Socket::~Socket() {
#ifdef WIN32
    ::closesocket(sockDesc);
#else
    ::close(sockDesc);
#endif
    sockDesc = -1;
}

string Socket::getLocalAddress() throw(SocketException) {
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
        throw SocketException("Fetch of local address failed (getsockname())", true);
    }
    return inet_ntoa(addr.sin_addr);
}

unsigned short Socket::getLocalPort() throw(SocketException) {
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
        throw SocketException("Fetch of local port failed (getsockname())", true);
    }
    return ntohs(addr.sin_port);
}

void Socket::setLocalPort(unsigned short localPort) throw(SocketException) {
    // Bind the socket to its port
    sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(localPort);

    if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) {
        throw SocketException("Set of local port failed (bind())", true);
    }
}

void Socket::setLocalAddressAndPort(const string &localAddress,
                                    unsigned short localPort) throw(SocketException) {
    // Get the address of the requested host
    addrinfo *localAddr;
    fillAddr(localAddress, localPort, &localAddr);

    if (bind(sockDesc, localAddr->ai_addr, localAddr->ai_addrlen) < 0) {
        throw SocketException("Set of local address and port failed (bind())", true);
    }
    freeaddrinfo(localAddr);
}

void Socket::cleanUp() throw(SocketException) {
#ifdef WIN32
    if (WSACleanup() != 0) {
      throw SocketException("WSACleanup() failed");
    }
#endif
}

unsigned short Socket::resolveService(const string &service,
                                      const string &protocol) {
    struct servent *serv;        /* Structure containing service information */

    if ((serv = getservbyname(service.c_str(), protocol.c_str())) == NULL)
        return atoi(service.c_str());  /* Service is port number */
    else
        return ntohs(serv->s_port);    /* Found port (network byte order) by name */
}

// CommunicatingSocket Code

CommunicatingSocket::CommunicatingSocket(int type, int protocol)
throw(SocketException) : Socket(type, protocol) {
}

CommunicatingSocket::CommunicatingSocket(int newConnSD) : Socket(newConnSD) {
}

void CommunicatingSocket::connect(const string &foreignAddress,
                                  unsigned short foreignPort) throw(SocketException) {
    // Get the address of the requested host
    struct addrinfo *servinfo, *p;
    fillAddr(foreignAddress, foreignPort, &servinfo);
    for (p = servinfo; p != NULL; p = p->ai_next) {
        cout << "Results: " << endl;
        if ((sockDesc = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            cerr << "Socket" << endl;
            continue;
        }

        if (::connect(sockDesc, p->ai_addr, p->ai_addrlen) == -1) {
            cout << "Error123" << endl;
            cerr << "Error connecting" << endl;
            // throw SocketException("Connect failed (connect())", true);
            continue;
        }
        break;
    }
    cout << "Freeing addr" << endl;
    //freeaddrinfo(&hints);
    freeaddrinfo(servinfo);
    //freeaddrinfo(p);
}

void CommunicatingSocket::send(const void *buffer, int bufferLen)
throw(SocketException) {
    if (::send(sockDesc, (raw_type *) buffer, bufferLen, 0) < 0) {
        throw SocketException("Send failed (send())", true);
    }
}

int CommunicatingSocket::recv(void *buffer, int bufferLen)
throw(SocketException) {
    int rtn;
    if ((rtn = ::recv(sockDesc, (raw_type *) buffer, bufferLen, 0)) < 0) {
        throw SocketException("Received failed (recv())", true);
    }

    return rtn;
}

string CommunicatingSocket::getForeignAddress()
throw(SocketException) {
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getpeername(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
        throw SocketException("Fetch of foreign address failed (getpeername())", true);
    }
    return inet_ntoa(addr.sin_addr);
}

unsigned short CommunicatingSocket::getForeignPort() throw(SocketException) {
    sockaddr_in addr;
    unsigned int addr_len = sizeof(addr);

    if (getpeername(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
        throw SocketException("Fetch of foreign port failed (getpeername())", true);
    }
    return ntohs(addr.sin_port);
}

// TCPSocket Code

TCPSocket::TCPSocket()
throw(SocketException) : CommunicatingSocket(SOCK_STREAM,
                                             IPPROTO_TCP) {
}

TCPSocket::TCPSocket(const string &foreignAddress, unsigned short foreignPort)
throw(SocketException) : CommunicatingSocket(SOCK_STREAM, IPPROTO_TCP) {
    connect(foreignAddress, foreignPort);
}

TCPSocket::TCPSocket(int newConnSD) : CommunicatingSocket(newConnSD) {
}

// TCPServerSocket Code

TCPServerSocket::TCPServerSocket(unsigned short localPort, int queueLen)
throw(SocketException) : Socket(SOCK_STREAM, IPPROTO_TCP) {
    setLocalPort(localPort);
    setListen(queueLen);
}

TCPServerSocket::TCPServerSocket(const string &localAddress,
                                 unsigned short localPort, int queueLen)
throw(SocketException) : Socket(SOCK_STREAM, IPPROTO_TCP) {
    setLocalAddressAndPort(localAddress, localPort);
    setListen(queueLen);
}

TCPSocket *TCPServerSocket::accept() throw(SocketException) {
    int newConnSD;
    if ((newConnSD = ::accept(sockDesc, NULL, 0)) < 0) {
        throw SocketException("Accept failed (accept())", true);
    }

    return new TCPSocket(newConnSD);
}

void TCPServerSocket::setListen(int queueLen) throw(SocketException) {
    if (listen(sockDesc, queueLen) < 0) {
        throw SocketException("Set listening socket failed (listen())", true);
    }
}