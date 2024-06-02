######################################################################
# https://stackoverflow.com/questions/20824194/cmake-with-google-protocol-buffers

######################################################################

cmake_policy(SET CMP0111 NEW)

set(Protobuf_USE_STATIC_LIBS ON)
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

list(TRANSFORM protoFiles PREPEND "../../protofiles/${myDir}/")
list(TRANSFORM protoFiles APPEND ".proto")

# https://github.com/protocolbuffers/protobuf/issues/14576
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set_target_properties(protobuf::protoc PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/protoc.sh" )
endif()

######################################################################

protobuf_generate_cpp(ProtoSources ProtoHeaders ${protoFiles})

set(targetName "steamdatabase-protobufs-${myDir}")
add_library("${targetName}" STATIC ${ProtoSources} ${ProtoHeaders})
target_link_libraries("${targetName}" INTERFACE "${Protobuf_LIBRARIES}")
target_include_directories("${targetName}" PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
