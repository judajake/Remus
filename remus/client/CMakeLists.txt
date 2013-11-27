project(Remus_Client)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${ZeroMQ_INCLUDE_DIR})

set(headers
    Client.h
    Job.h
    JobDataRequest.h
    JobFileRequest.h
    JobResult.h
    JobStatus.h
    ServerConnection.h
    )

remus_public_headers(${headers})
