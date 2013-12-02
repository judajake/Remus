
#include "BasicRPCService_Dispatcher.pb.h"


#include <remus/client/Job.h>
#include <remus/client/JobDataRequest.h>
#include <remus/client/JobFileRequest.h>
#include <remus/client/JobResult.h>
#include <remus/client/JobStatus.h>

#include <remus/common/Message.h>

namespace detail
{
//------------------------------------------------------------------------------
inline std::string to_string(const boost::uuids::uuid& id)
{
  //call the boost to_string method in uuid_io
  return boost::lexical_cast<std::string>(id);
}

//------------------------------------------------------------------------------
inline boost::uuids::uuid to_uuid(const std::string& str)
{
  return boost::lexical_cast<boost::uuids::uuid>(str);
}
}

namespace remus {
namespace from {

//------------------------------------------------------------------------------
remus::common::MeshIOType ProtoMeshIOType(const ::MeshIOType& proto_type)
{
  remus::MESH_INPUT_TYPE in_type =
      static_cast<remus::MESH_INPUT_TYPE>(proto_type.in_type());
  remus::MESH_OUTPUT_TYPE out_type =
      static_cast<remus::MESH_OUTPUT_TYPE>(proto_type.out_type());
  return remus::common::MeshIOType(in_type,out_type);
}

//------------------------------------------------------------------------------
boost::uuids::uuid ProtoUUID(const ::UUID& proto_uuid)
{
  return detail::to_uuid(proto_uuid.id());
}

//------------------------------------------------------------------------------
remus::common::MeshIOType ProtoMeshIOType(const Request* req)
{
  remus::common::MeshIOType mesh_type;
  if(req->has_filerequest())
    {
    mesh_type = ProtoMeshIOType(req->filerequest().type());
    }
  else if(req->has_datarequest())
    {
    mesh_type = ProtoMeshIOType(req->datarequest().type());
    }
  return mesh_type;
}

//------------------------------------------------------------------------------
remus::client::Job ProtoJob(const Request* req)
{
  remus::common::MeshIOType mesh_type = ProtoMeshIOType(req->job().type());
  boost::uuids::uuid jid = ProtoUUID(req->job().job_id());
  return remus::client::Job(jid,mesh_type);
}

//------------------------------------------------------------------------------
remus::client::Job ProtoJob(const Response* resp)
{
  remus::common::MeshIOType mesh_type = ProtoMeshIOType(resp->job().type());
  boost::uuids::uuid jid = ProtoUUID(resp->job().job_id());
  return remus::client::Job(jid,mesh_type);
}

//------------------------------------------------------------------------------
remus::client::JobStatus ProtoJobStatus(const Response* resp)
{
  const ::JobStatus& proto_status = resp->status();
  boost::uuids::uuid jid = ProtoUUID(proto_status.job_id());
  remus::STATUS_TYPE stype = remus::STATUS_TYPE(proto_status.stype());

  remus::client::JobStatus my_status(jid,stype);
  if(stype == remus::IN_PROGRESS)
    {
    const ::JobProgress& proto_prog = proto_status.progress();
    //we need to extract the exact progress
    remus::client::JobProgress progress(proto_prog.value(),
                                        proto_prog.message());
    my_status = remus::client::JobStatus(jid,progress);
    }
  return my_status;
}

//------------------------------------------------------------------------------
remus::client::JobResult ProtoJobResult(const Response* resp)
{
  const ::JobResult& proto_result = resp->result();

  boost::uuids::uuid jid = ProtoUUID(proto_result.job_id());
  return remus::client::JobResult(jid,proto_result.data());
}


} } //remus::from

namespace remus {
namespace to {

//------------------------------------------------------------------------------
void ProtoMeshType(const remus::common::MeshIOType& type,
                   ::MeshIOType* proto_type)
{
  proto_type->set_in_type(type.inputType());
  proto_type->set_out_type(type.outputType());
}

//------------------------------------------------------------------------------
void ProtoUUID(const boost::uuids::uuid& uuid, ::UUID* proto_id)
{
  proto_id->set_id( detail::to_string(uuid) );
}

//------------------------------------------------------------------------------
void ProtoJobDataRequest(const remus::client::JobDataRequest& req,
                         ::JobDataRequest *proto_datareq)
{
  ProtoMeshType(req.type(),proto_datareq->mutable_type());
  proto_datareq->set_jobinfo(req.jobInfo());
}


//------------------------------------------------------------------------------
void ProtoJob(const remus::client::Job& j, ::Job *proto_job)
{
  ProtoUUID(j.id(),proto_job->mutable_job_id());
  ProtoMeshType(j.type(),proto_job->mutable_type());
}

//------------------------------------------------------------------------------
void ProtoStatus(const remus::client::JobStatus& js, ::JobStatus *proto_status)
{
  ProtoUUID(js.JobId,proto_status->mutable_job_id());
  proto_status->set_stype( JobStatus::STATUS_MODE(js.Status) );

  ::JobProgress* my_progress = proto_status->mutable_progress();
  my_progress->set_value(js.Progress.value());
  my_progress->set_message(js.Progress.message());
}


//------------------------------------------------------------------------------
void ProtoResult(const remus::client::JobResult& jr, ::JobResult* proto_result)
{
  ProtoUUID(jr.JobId, proto_result->mutable_job_id());
  proto_result->set_data( jr.Data );
}

} } //namespace remus::to
