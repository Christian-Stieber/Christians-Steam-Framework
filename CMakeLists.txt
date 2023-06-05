######################################################################
#
# Use with a command like
#   cmake -S . -B build/Debug -D CMAKE_BUILD_TYPE=Debug
#   cmake --build build/Debug

cmake_minimum_required(VERSION 3.23)
set (CMAKE_CXX_STANDARD 20)
project(SteamBot CXX)

######################################################################
# https://developpaper.com/correct-use-of-protobuf-in-cmake/
#
# ToDo: does it really have to be that complicated?
# Took hours to set up...

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

cmake_path(SET protoDir "${CMAKE_CURRENT_SOURCE_DIR}")
cmake_path(APPEND protoDir "steamdatabase" "protobufs")

list(APPEND protoFiles steam/steammessages_base)
list(APPEND protoFiles steam/steammessages_clientserver)
list(APPEND protoFiles steam/steammessages_clientserver_login)
list(APPEND protoFiles steam/steammessages_clientserver_2)
list(APPEND protoFiles steam/enums)
list(APPEND protoFiles steam/encrypted_app_ticket)
list(APPEND protoFiles steam/steammessages_unified_base.steamclient)
list(APPEND protoFiles steam/steammessages_player.steamclient)
list(APPEND protoFiles steam/steammessages_clientserver_uds)
list(APPEND protoFiles steam/steammessages_clientserver_friends)

list(TRANSFORM protoFiles PREPEND "${protoDir}/")
list(TRANSFORM protoFiles APPEND ".proto")

cmake_path(SET protoSteamDir "${protoDir}")
cmake_path(APPEND protoSteamDir "steam")

foreach(protoFile ${protoFiles})
   file(RELATIVE_PATH protoRelativeFile "${CMAKE_CURRENT_SOURCE_DIR}" "${protoFile}")
   cmake_path(GET protoFile PARENT_PATH protoFileSourceDir)
   cmake_path(GET protoFile FILENAME protoFileName)
   cmake_path(GET protoRelativeFile PARENT_PATH protoFileDir)
   cmake_path(GET protoRelativeFile STEM LAST_ONLY protoFileStem)

   cmake_path(SET protoFileTargetDir "${CMAKE_CURRENT_BINARY_DIR}")
   cmake_path(APPEND protoFileTargetDir "${protoRelativeFile}")
   cmake_path(REMOVE_FILENAME protoFileTargetDir)
   file(MAKE_DIRECTORY "${protoFileTargetDir}")

   cmake_path(SET finalName "${protoFileTargetDir}")
   cmake_path(APPEND finalName "${protoFileStem}")

   execute_process(COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} ${PROTO_FLAGS} "-I${protoFileSourceDir}" "-I${protoDir}" "-I${protoSteamDir}" --cpp_out=${protoFileTargetDir} ${protoFileName}
                   WORKING_DIRECTORY "${protoFileSourceDir}"
                   ERROR_FILE "${finalName}.errors")
   execute_process(COMMAND "grep" -v "libprotobuf WARNING .* No syntax specified for the proto file:" INPUT_FILE "${finalName}.errors" OUTPUT_FILE "/dev/stderr")

   list(APPEND PROTO_SRCS "${finalName}.pb.cc")
   list(APPEND PROTO_HDRS "${finalName}.pb.h")
endforeach()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

######################################################################

SET(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.81.0 REQUIRED COMPONENTS context coroutine system log iostreams fiber)
include_directories(${Boost_INCLUDE_DIRS})

list(APPEND SRCS Boost)

######################################################################

find_package(OpenSSL 3 REQUIRED)

######################################################################

list(APPEND SRCS Main WorkingDir HTTPClient Universe Random Config Base64 AsioThread DestructMonitor JobID)
list(APPEND SRCS Client/Client Client/Waiter Client/Whiteboard Client/Messageboard Client/Module Client/DataFile)
list(APPEND SRCS Connection/Endpoint Connection/Serialize Connection/Base Connection/TCP Connection/Message Connection/Encrypted)
list(APPEND SRCS OpenSSL/Exception OpenSSL/SHA1 OpenSSL/RSA OpenSSL/AESBase OpenSSL/AES OpenSSL/AESHMAC OpenSSL/Random)
list(APPEND SRCS Steam/OSType Steam/KeyValue)
list(APPEND SRCS Steam/MachineInfo Steam/MachineInfo/Linux)
list(APPEND SRCS Web/URLEncode)
list(APPEND SRCS Web/Cookies)
list(APPEND SRCS Helpers/URLs)
list(APPEND SRCS Helpers/HTML)
list(APPEND SRCS Modules/Connection)
list(APPEND SRCS Modules/MultiPacket)
list(APPEND SRCS Modules/Heartbeat)
list(APPEND SRCS Modules/UnifiedMessageClient)
list(APPEND SRCS Modules/UnifiedMessageServer)
list(APPEND SRCS Modules/OwnedGames)
list(APPEND SRCS Modules/LicenseList)
list(APPEND SRCS Modules/ClientAppList)
list(APPEND SRCS Modules/PersonaState)
list(APPEND SRCS Modules/SteamGuard)
list(APPEND SRCS Modules/WebSession)
list(APPEND SRCS Modules/PlayGames)
list(APPEND SRCS Modules/IdleGame)
list(APPEND SRCS UI/UI)
list(APPEND SRCS Modules/Login/Login Modules/Login/ClientLogonMessage Modules/Login/SteamGuard)
list(APPEND SRCS Modules/GetBadgeData/ParseBadgePage Modules/GetBadgeData/GetBadgeData)

# list(APPEND SRCS Tests/Asio Tests/Whiteboard Tests/Messageboard Tests/HTTPClient Tests/GetCMList)

list(APPEND SRCS WebAPI/WebAPI WebAPI/ISteamDirectory/GetCMList)

include_directories("Headers")
include_directories("Christians-HTML-Parser/Headers")

######################################################################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")	#  -fsanitize=address

######################################################################

message("${SRCS}")

list(TRANSFORM SRCS PREPEND "Sources/")
list(TRANSFORM SRCS APPEND ".cpp")

add_executable(SteamBot ${SRCS} ${PROTO_SRCS})
target_compile_features(SteamBot PUBLIC cxx_std_20)

target_link_libraries(SteamBot pthread systemd ${Boost_LIBRARIES})
target_link_libraries(SteamBot ${Protobuf_LIBRARIES})
target_link_libraries(SteamBot ${OPENSSL_LIBRARIES})
target_link_libraries(SteamBot z)
target_link_libraries(SteamBot atomic)
