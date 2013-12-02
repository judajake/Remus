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

#include "RPCService.h"
#include "RPCUtil.h"

#include "BasicRPCService_Dispatcher.pb.h"
#include <protocall/runtime/servicemanager.h>
#include "zmqCommunicatorChannel.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <remus/server/detail/ActiveJobs.h>
#include <remus/server/detail/JobQueue.h>
#include <remus/server/detail/WorkerPool.h>

//------------------------------------------------------------------------------
bool BasicRPCServiceImpl::canHandleMeshType(
                                    remus::common::MeshIOType mesh_type) const
{
  return this->Server->WorkerFactory.haveSupport(mesh_type) ||
         this->Server->WorkerPool->haveWaitingWorker(mesh_type);
}


//------------------------------------------------------------------------------
void BasicRPCServiceImpl::canMesh(const Request* req,
                                 Response* res,
                                 google::protobuf::Closure* done)
{
  const remus::common::MeshIOType mesh_type = remus::from::ProtoMeshIOType(req);
  res->set_valid( this->canHandleMeshType(mesh_type) );
  done->Run();
}

//------------------------------------------------------------------------------
void BasicRPCServiceImpl::submitJob(const Request* req,
                                   Response* res,
                                   google::protobuf::Closure* done)
{
  const remus::common::MeshIOType mesh_type = remus::from::ProtoMeshIOType(req);

  const bool mesh_valid = this->canHandleMeshType(mesh_type);
  res->set_valid( mesh_valid );
  if(mesh_valid)
  {
    //generate an UUID
    const boost::uuids::uuid jobUUID = this->Server->UUIDGenerator();
    //create a new job to place on the queue
    //This call will invalidate the msg as we are going to move the data
    //to another message to await sending to the worker

    //really hacky
    remus::common::Message msg(mesh_type,
                               remus::MAKE_MESH,
                               req->datarequest().jobinfo());
    this->Server->QueuedJobs->addJob(jobUUID,msg);

    //return the UUID

    const remus::client::Job validJob(jobUUID,mesh_type);
    remus::to::ProtoJob(validJob,res->mutable_job());
  }

  done->Run();
}

//------------------------------------------------------------------------------
void BasicRPCServiceImpl::jobStatus(const Request* req,
                                   Response* res,
                                   google::protobuf::Closure* done)
{
  remus::client::Job job = remus::from::ProtoJob(req);
  remus::client::JobStatus js(job.id(),remus::INVALID_STATUS);
  if(this->Server->QueuedJobs->haveUUID(job.id()))
    {
    js.Status = remus::QUEUED;
    }
  else if(this->Server->ActiveJobs->haveUUID(job.id()))
    {
    js = this->Server->ActiveJobs->status(job.id());
    }

  remus::to::ProtoStatus(js, res->mutable_status() );
  res->set_valid(js.Status != remus::INVALID_STATUS);

  done->Run();
}

//------------------------------------------------------------------------------
void BasicRPCServiceImpl::retrieveResults(const Request* req,
                                         Response* res,
                                         google::protobuf::Closure* done)
{
  //go to the active jobs list and grab the mesh result if it exists
  res->set_valid( req->has_job() );
  if(req->has_job())
    {
    remus::client::Job job = remus::from::ProtoJob(req);
    remus::client::JobResult result(job.id());
    if( this->Server->ActiveJobs->haveUUID(job.id()) &&
        this->Server->ActiveJobs->haveResult(job.id()))
      {
      result = this->Server->ActiveJobs->result(job.id());
      //for now we remove all references from this job being active
      this->Server->ActiveJobs->remove(job.id());
      }
    remus::to::ProtoResult(result, res->mutable_result());
    }

  done->Run();
}

//------------------------------------------------------------------------------
void BasicRPCServiceImpl::terminate(const Request* req,
                                   Response* res,
                                   google::protobuf::Closure* done)
{
  std::cout << "terminate" << std::endl;
  //todo implement
}


//------------------------------------------------------------------------------
bool BasicRPCServer::startBrokering()
{
  typedef ProtoCall::Runtime::zmqCommunicatorChannel::socketIdentity sIdentity;

  this->StartCatchingSignals();

  //set up the client rpc channel and register it with protobuff services
  ProtoCall::Runtime::zmqCommunicatorChannel cliet_channel(this->ClientQueries);
  ProtoCall::Runtime::ServiceManager *mgr =
                                ProtoCall::Runtime::ServiceManager::instance();
  BasicRPCServiceImpl service(this);
  BasicRPCService::Dispatcher dispatcher(&service);
  mgr->registerService(&dispatcher);

  zmq::pollitem_t items[2] = {
      { this->ClientQueries,  0, ZMQ_POLLIN, 0 },
      { this->WorkerQueries, 0, ZMQ_POLLIN, 0 } };

  //  Process messages from both sockets
  while (true)
    {
    zmq::poll(&items[0], 2, remus::HEARTBEAT_INTERVAL);

    const boost::posix_time::ptime hbTime =
                          boost::posix_time::second_clock::local_time();

    if (items[0].revents & ZMQ_POLLIN)
      {
      //we need to strip the client address from the message
      //use some hacky interop for now
      zmq::socketIdentity clientIdentity = zmq::address_recv(this->ClientQueries);
      sIdentity cIdentity(clientIdentity.data(),clientIdentity.size());

      //Note the contents of the message isn't valid
      //after the DetermineJobQueryResponse call
      cliet_channel.push_socket_identity(cIdentity);

      cliet_channel.receive(false);

      cliet_channel.clear_socket_identities();

      }
    if (items[1].revents & ZMQ_POLLIN)
      {
      //a worker is registering
      //we need to strip the worker address from the message
      zmq::socketIdentity workerIdentity = zmq::address_recv(this->WorkerQueries);

      //Note the contents of the message isn't valid
      //after the DetermineWorkerResponse call
      remus::common::Message message(this->WorkerQueries);
      this->DetermineWorkerResponse(workerIdentity,message);

      //refresh all jobs for a given worker with a new expiry time
      this->ActiveJobs->refreshJobs(workerIdentity);

      //refresh the worker if it is actuall in the pool instead of doing a job
      this->WorkerPool->refreshWorker(workerIdentity);
      }

    //mark all jobs whose worker haven't sent a heartbeat in time
    //as a job that failed.
    this->ActiveJobs->markExpiredJobs(hbTime);

    //purge all pending workers with jobs that haven't sent a heartbeat
    this->WorkerPool->purgeDeadWorkers(hbTime);

    //see if we have a worker in the pool for the next job in the queue,
    //otherwise as the factory to generat a new worker to handle that job
    this->FindWorkerForQueuedJob();
    }

  //this should never be hit, but just incase lets make sure we close
  //down all workers.
  this->TerminateAllWorkers();

  this->StopCatchingSignals();
}
