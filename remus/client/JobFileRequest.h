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

#ifndef remus_client_JobFileRequest_h
#define remus_client_JobFileRequest_h

#include <string>
#include <sstream>

#include <remus/common/MeshIOType.h>

//JobFileRequest is a efficient way to send a file and collection of arguments
//to a worker.
//If the client, server, and worker are all on the same machine we will
//send only the file name and arguments to the worker.
//at any point if the client, server, or worker are on different machines
//we will read the file and store the contents and send that as a binary
//blob
//So if the client and server are on the same machine, when the server is
//about to dispatch the job to a remote worker it will read the file and
//send the contents of the file to the server
//
//
//Note: the current issues with this behavior is that we expect
//that the server will have read privileges on the file that we are sending


//A job request has two purposes. First it is sent to the server to determine
//if the mesh type and data model that the job request has is supported.
//Secondly it is used to transport the actual job to the server

//Note: Currently no server supports the ability to restrict a mesh request
//to a single named mesher. This option is expected to be supported in the future.
//For now we replicate this feature by making <MeshIOType,inputMeshDataType> be
//unique for each mesher
namespace remus{
namespace client{
class JobFileRequest
{
public:

  //Construct a job request with only a mesher type given. This is used to
  //when asking a server if it supports a type of mesh
  JobFileRequest(remus::MESH_INPUT_TYPE inputFileType,
             remus::MESH_OUTPUT_TYPE outputMeshIOType):
    CombinedType(inputFileType,outputMeshIOType),
    FilePath(),
    CommandArgs()
    {
    }

  //Construct a job request with a mesh type and a file and arguments
  //to run the job. This is used when submitting a job from the client to the server.
  JobFileRequest(remus::MESH_INPUT_TYPE inputFileType,
             remus::MESH_OUTPUT_TYPE outputMeshIOType,
             const std::string& path,
             const std::string& args):
    CombinedType(inputFileType,outputMeshIOType),
    FilePath(path),
    CommandArgs(args)
    {
    }

  //Construct a job request with the given types held inside the remus::common::MeshIOType object
  explicit JobFileRequest(remus::common::MeshIOType combinedType):
    CombinedType(combinedType),
    FilePath(),
    CommandArgs()
    {
    }

  //Construct a job request with the given types held inside the remus::common::MeshIOType object
  //and a path and args for a file
  JobFileRequest(remus::common::MeshIOType combinedType,
                 const std::string& path,
                 const std::string& args):
    CombinedType(combinedType),
    FilePath(path),
    CommandArgs(args)
    {

    }

  //constructs a variable that represents the combination of the input
  //and output type as a single integer
  remus::common::MeshIOType type() const { return this->CombinedType; }

  remus::MESH_OUTPUT_TYPE outputType() const { return CombinedType.outputType(); }
  remus::MESH_INPUT_TYPE inputType() const { return CombinedType.inputType(); }


  const std::string& filePath() const { return this->FilePath; }
  const std::string& commandArgs() const { return this->CommandArgs; }

private:
  remus::common::MeshIOType CombinedType;
  std::string FilePath;
  std::string CommandArgs;
};

//------------------------------------------------------------------------------
inline std::string to_string(const remus::client::JobFileRequest& request)
{
  //convert a request to a string, used as a hack to serialize
  //encoding is simple, contents newline separated
  std::stringstream buffer;
  buffer << request.type() << std::endl;
  buffer << request.filePath().length() << std::endl;
  remus::internal::writeString(buffer, request.filePath());
  buffer << request.commandArgs().length() << std::endl;
  remus::internal::writeString(buffer, request.commandArgs());
  return buffer.str();
}


//------------------------------------------------------------------------------
inline remus::client::JobFileRequest to_JobFileRequest(const std::string& msg)
{
  //convert a job detail from a string, used as a hack to serialize
  std::stringstream buffer(msg);

  int pathLen, argsLen;
  std::string path;
  std::string args;

  remus::common::MeshIOType jobRequirements;

  buffer >> jobRequirements;
  buffer >> pathLen;
  path = remus::internal::extractString(buffer,pathLen);
  buffer >> argsLen;
  path = remus::internal::extractString(buffer,argsLen);

  return remus::client::JobFileRequest(jobRequirements,path,args);
}


//------------------------------------------------------------------------------
inline remus::client::JobFileRequest to_JobFileRequest(const char* data, int size)
{
  //convert a job request from a string, used as a hack to serialize
  std::string temp(size,char());
  memcpy(const_cast<char*>(temp.c_str()),data,size);
  return to_JobFileRequest( temp );
}


}
}

#endif
