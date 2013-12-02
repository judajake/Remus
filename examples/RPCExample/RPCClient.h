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

#ifndef RPCClient_h
#define RPCClient_h

#include <google/protobuf/stubs/common.h>
#include "BasicRPCService_Dispatcher.pb.h"
#include "zmqCommunicatorChannel.h"
#include "RPCUtil.h"

#include <string>
#include <vector>

#include <remus/client/Job.h>
#include <remus/client/JobResult.h>
#include <remus/client/JobDataRequest.h>
#include <remus/client/JobFileRequest.h>
#include <remus/client/JobStatus.h>

#include <remus/common/Message.h>
#include <remus/common/Response.h>
#include <remus/common/zmqHelper.h>
#include <remus/client/ServerConnection.h>

#include <iostream>

class RPCClient
{
public:
  //connect to a given host on a given port with tcp
  explicit RPCClient(const remus::client::ServerConnection& conn);

  //Submit a request to the server to see if it support the requirements
  //of a given job request
  bool canMesh(const remus::client::JobDataRequest& request);
  bool canMesh(const remus::client::JobFileRequest& request);

  //Submit a job to the server.
  remus::client::Job submitJob(const remus::client::JobDataRequest& request);
  remus::client::Job submitJob(const remus::client::JobFileRequest& request);

  //Given a remus Job object returns the status of the job
  remus::client::JobStatus jobStatus(const remus::client::Job& job);

  //Return job result of of a give job
  remus::client::JobResult retrieveResults(const remus::client::Job& job);

  //attempts to terminate a given job, will kill the worker of a job
  //if the job is still pending. If the job has been finished and the results
  //are on the server the results will be deleted.
  remus::client::JobStatus terminate(const remus::client::Job& job);

private:
  friend class google::protobuf::Closure;
  //method that we use for the callback to set the spin lock flag
  void CallbackMethod(){ }

  zmq::context_t Context;
  zmq::socket_t ServerSocket;
  ProtoCall::Runtime::zmqCommunicatorChannel Channel;
  BasicRPCService::Proxy ServerProxy;
  //ivars to convert the async rpc methods to a sync api
  Response  RPCResponse;
  boost::shared_ptr<google::protobuf::Closure> Callback;

  //poll items
  zmq::pollitem_t PollItems;

};

//------------------------------------------------------------------------------
RPCClient::RPCClient(const remus::client::ServerConnection &conn):
  Context(1),
  ServerSocket(Context, ZMQ_REQ),
  Channel(ServerSocket),
  ServerProxy(&Channel),
  RPCResponse(),
  Callback(google::protobuf::NewPermanentCallback(this,
                                                &RPCClient::CallbackMethod ))
{
  zmq::connectToAddress(this->ServerSocket,conn.endpoint());
  zmq::pollitem_t temp = { this->ServerSocket,  0, ZMQ_POLLIN, 0 };
  this->PollItems = temp;
}

//------------------------------------------------------------------------------
bool RPCClient::canMesh(const remus::client::JobDataRequest& request)
{
  Request   rpc_req;

  remus::to::ProtoJobDataRequest(request, rpc_req.mutable_datarequest());
  this->ServerProxy.canMesh(&rpc_req, &this->RPCResponse, this->Callback.get());

  zmq::poll(&PollItems, 1, -1); //sync

  this->Channel.receive();

  return this->RPCResponse.valid();
}

//------------------------------------------------------------------------------
remus::client::Job RPCClient::submitJob(const remus::client::JobDataRequest& request)
{

  Request   rpc_req;

  remus::to::ProtoJobDataRequest(request, rpc_req.mutable_datarequest());
  this->ServerProxy.submitJob(&rpc_req, &this->RPCResponse, this->Callback.get());

  zmq::poll(&PollItems, 1, -1); //sync

  this->Channel.receive();

  return remus::from::ProtoJob(&this->RPCResponse);
}

//------------------------------------------------------------------------------
remus::client::JobStatus RPCClient::jobStatus(const remus::client::Job& job)
{
  Request   rpc_req;

  remus::to::ProtoJob(job, rpc_req.mutable_job());
  this->ServerProxy.jobStatus(&rpc_req, &this->RPCResponse, this->Callback.get());

  zmq::poll(&PollItems, 1, -1); //sync

  this->Channel.receive();

  return remus::from::ProtoJobStatus(&this->RPCResponse);
}

//------------------------------------------------------------------------------
remus::client::JobResult RPCClient::retrieveResults(const remus::client::Job& job)
{
  Request   rpc_req;

  remus::to::ProtoJob(job, rpc_req.mutable_job());
  this->ServerProxy.retrieveResults(&rpc_req, &this->RPCResponse, this->Callback.get());

  zmq::poll(&PollItems, 1, -1); //sync

  this->Channel.receive();

  return remus::from::ProtoJobResult(&this->RPCResponse);
}

//------------------------------------------------------------------------------
remus::client::JobStatus RPCClient::terminate(const remus::client::Job& job)
{
  //todo implement
  return remus::client::JobStatus(job.id(),remus::INVALID_STATUS);
}

#endif
