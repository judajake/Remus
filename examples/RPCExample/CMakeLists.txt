
find_package(protobuf REQUIRED)
find_package(ProtoCall REQUIRED)

set(SRCS_Server
   serverMain.cxx
   RPCService.cpp
   )

set(SRCS_Client
   clientMain.cxx
   )

protocallc(_common_source
  _service_source
  _proxy_source
  _include_dir
  "basic.proto"
  )

protocall_communicator( zmq communicator_srcs )

include_directories(${protobuf_INCLUDE_DIRS}
                    ${ProtoCall_INCLUDE_DIRS}
                    ${_include_dir}
                    )

add_library(RPCRuntime ${_common_source} ${communicator_srcs})
target_link_libraries(RPCRuntime LINK_PRIVATE ${ProtoCall_LIBRARIES})

add_executable(RPCClient ${_proxy_source} ${SRCS_Client})
target_link_libraries(RPCClient RPCRuntime RemusCommon)

add_executable(RPCServer ${_service_source} ${SRCS_Server} )
target_link_libraries(RPCServer RPCRuntime RemusServer)
