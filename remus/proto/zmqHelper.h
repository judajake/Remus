//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef remus_proto_zmqHelper_h
#define remus_proto_zmqHelper_h

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <boost/lexical_cast.hpp>

//We now provide our own zmq.hpp since it has been removed from zmq 3, and
//made its own project
#include <remus/proto/zmq.hpp>
#include <remus/proto/zmqSocketIdentity.h>
#include <remus/proto/zmqSocketInfo.h>

//inject some basic zero MQ helper functions into the namespace
namespace zmq
{

inline bool address_send(zmq::socket_t & socket, const zmq::SocketIdentity& address)
{
  zmq::message_t message(address.size());
  std::copy(address.data(),
            address.data()+address.size(),
            static_cast<unsigned char*>(message.data()));
  return socket.send(message);
}

inline zmq::SocketIdentity address_recv(zmq::socket_t& socket)
{
  zmq::message_t message;
  socket.recv(&message);
  return zmq::SocketIdentity((char*)message.data(),message.size());
}


inline void connectToAddress(zmq::socket_t &socket,const std::string &endpoint)
{
  socket.connect(endpoint.c_str());
}

template<typename T>
inline void connectToAddress(zmq::socket_t &socket,const zmq::socketInfo<T> &sInfo)
{
  socket.connect(sInfo.endpoint().c_str());
}

//A wrapper around zeroMQ send. When we call the standard send call
//from a Qt class we experience high number of system level interrupts which
//cause zero to throw an exception when we are sending a blocking message.
//When sending a blocking message we will try a couple of times before
//giving up
//In the future we need to change the client server
//communication in Remus to be async instead of req/reply based.
inline bool send_harder(zmq::socket_t& socket, zmq::message_t& message, int flags=0)
{
  bool sent = false;
  short tries = 0;
  while(!sent && tries < 5)
    {
    try{sent = socket.send(message,flags);}
    catch(error_t){ ++tries; }
    }
  return sent;
}

//A wrapper around zeroMQ recv. When we call the standard recv call
//from a Qt class we experience high number of system level interrupts which
//cause zero to throw an exception when we are recv a blocking message.
//When recving a blocking message we will try a couple of times before
//giving up
//In the future we need to change the client server
//communication in Remus to be async instead of req/reply based.
inline bool recv_harder(zmq::socket_t& socket, zmq::message_t* message, int flags=0)
{
  bool recieved = false;
  short tries = 0;
  while(!recieved && tries < 5)
    {
    try{recieved = socket.recv(message,flags);}
    catch(error_t){ ++tries; }
    }
  return recieved;
}

//we presume that every message needs to be stripped
//as we make everything act like a req/rep and pad
//a null message on everything
inline void removeReqHeader(zmq::socket_t& socket)
{
  int socketType;
  std::size_t socketTypeSize = sizeof(socketType);
  socket.getsockopt(ZMQ_TYPE,&socketType,&socketTypeSize);
  if(socketType != ZMQ_REQ && socketType != ZMQ_REP)
    {
    zmq::message_t reqHeader;
    socket.recv(&reqHeader);
    }
}

//if we are not a req or rep socket make us look like one
 inline void attachReqHeader(zmq::socket_t& socket)
{
  int socketType;
  std::size_t socketTypeSize = sizeof(socketType);
  socket.getsockopt(ZMQ_TYPE,&socketType,&socketTypeSize);
  if(socketType != ZMQ_REQ && socketType != ZMQ_REP)
    {
    zmq::message_t reqHeader(0);
    socket.send(reqHeader,ZMQ_SNDMORE);
    }
}

}

#endif // remus_proto_zmqHelper_h
