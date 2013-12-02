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

#ifndef BasicRPCServiceImpl_H_
#define BasicRPCServiceImpl_H_

#include "BasicRPCService.pb.h"
#include <remus/server/Server.h>

class BasicRPCServer;


class BasicRPCServiceImpl: public BasicRPCService
{
public:
  BasicRPCServiceImpl(BasicRPCServer* server):
    BasicRPCService(),
    Server(server)
  {
  }

protected:
  void canMesh(const Request* req, Response* res, google::protobuf::Closure* done);
  void submitJob(const Request* req, Response* res, google::protobuf::Closure* done);
  void jobStatus(const Request* req, Response* res, google::protobuf::Closure* done);
  void retrieveResults(const Request* req, Response* res, google::protobuf::Closure* done);
  void terminate(const Request* req, Response* res, google::protobuf::Closure* done);


  //helper methods
  bool canHandleMeshType(remus::common::MeshIOType mesh_type) const;

private:
  //a hack for proof concept a better model will need to be designed go forward
  BasicRPCServer* Server;
};


class BasicRPCServer: public remus::Server::Server
{
public:
  //hack for proof of concept
  friend class BasicRPCServiceImpl;

  BasicRPCServer():
    Server()
  {}

  explicit BasicRPCServer(const remus::server::WorkerFactory& factory):
    Server(factory)
  {}


  virtual bool startBrokering();
};

#endif
