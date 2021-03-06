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

#include <remus/server/ServerPorts.h>
#include <remus/proto/zmqHelper.h>

#include <boost/make_shared.hpp>

namespace
{
remus::server::PortConnection bindToTCPSocket(zmq::socket_t &socket,
                                          remus::server::PortConnection conn)
{
  //go through all ports, I hope the input port is inside the Ephemeral range
  int rc = -1;
  zmq::socketInfo<zmq::proto::tcp> socketInfo(conn.host(),conn.port());
  for(int i=socketInfo.port();i < 65535 && rc != 0; ++i)
    {
    socketInfo.setPort(i);
    //using the C syntax to skip having to catch the exception;
    rc = zmq_bind(socket.operator void *(),socketInfo.endpoint().c_str());
    }

  if(rc!=0)
    {
    throw zmq::error_t();
    }
  return remus::server::PortConnection(socketInfo);
}

}


namespace remus{
namespace server{

//------------------------------------------------------------------------------
ServerPorts::ServerPorts():
  Context( boost::make_shared<zmq::context_t>(2) ),
  Client(zmq::socketInfo<zmq::proto::tcp>("127.0.0.1",
                                          remus::SERVER_CLIENT_PORT)),
  Worker(zmq::socketInfo<zmq::proto::tcp>("127.0.0.1",
                                          remus::SERVER_WORKER_PORT))
{
  assert(remus::SERVER_CLIENT_PORT > 0 && remus::SERVER_CLIENT_PORT < 65536);
  assert(remus::SERVER_WORKER_PORT > 0 && remus::SERVER_WORKER_PORT < 65536);
  assert(remus::SERVER_CLIENT_PORT != remus::SERVER_WORKER_PORT);
}

//------------------------------------------------------------------------------
ServerPorts::ServerPorts(const std::string& clientHostName,
                         unsigned int clientPort,
                         const std::string& workerHostName,
                         unsigned int workerPort):
  Context( boost::make_shared<zmq::context_t>(2) ),
  Client(zmq::socketInfo<zmq::proto::tcp>(clientHostName,clientPort)),
  Worker(zmq::socketInfo<zmq::proto::tcp>(workerHostName,workerPort))
{
  assert(clientHostName.size() > 0);
  assert(clientPort > 0 && clientPort < 65536);

  assert(workerHostName.size() > 0);
  assert(workerPort > 0 && workerPort < 65536);

  assert(workerHostName != clientHostName && workerPort != clientPort);
}

//------------------------------------------------------------------------------
void ServerPorts::bindClient(zmq::socket_t* socket)
{
  if(this->Client.scheme() == zmq::proto::scheme_name(zmq::proto::tcp()))
    {
    this->Client = bindToTCPSocket(*socket,this->Client);
    }
  else
    {
    socket->bind(this->Client.endpoint().c_str());
    }
}

//------------------------------------------------------------------------------
void ServerPorts::bindWorker(zmq::socket_t* socket)
{
  if(this->Worker.scheme() == zmq::proto::scheme_name(zmq::proto::tcp()))
    {
    this->Worker = bindToTCPSocket(*socket,this->Worker);
    }
  else
    {
    socket->bind(this->Worker.endpoint().c_str());
    }
}

//end namespaces
}
}
