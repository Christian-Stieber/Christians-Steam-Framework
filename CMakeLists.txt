cmake_minimum_required(VERSION 3.23)

project(ChristiansSteamFramework CXX)
add_library(${PROJECT_NAME} STATIC)

include(CMakefile.Common)

######################################################################
# https://developpaper.com/correct-use-of-protobuf-in-cmake/
#
# ToDo: does it really have to be that complicated?
# Took hours to set up...

cmake_path(SET protoDir "${CMAKE_CURRENT_SOURCE_DIR}")
cmake_path(APPEND protoDir "steamdatabase" "protobufs")

list(APPEND protoFiles steam/steammessages_base)
list(APPEND protoFiles steam/steammessages_clientserver)
list(APPEND protoFiles steam/steammessages_clientserver_login)
list(APPEND protoFiles steam/steammessages_clientserver_2)
list(APPEND protoFiles steam/steammessages_clientserver_appinfo)
list(APPEND protoFiles steam/enums)
list(APPEND protoFiles steam/encrypted_app_ticket)
list(APPEND protoFiles steam/steammessages_unified_base.steamclient)
list(APPEND protoFiles steam/steammessages_player.steamclient)
list(APPEND protoFiles steam/steammessages_clientserver_uds)
list(APPEND protoFiles steam/steammessages_clientserver_friends)
list(APPEND protoFiles steam/steammessages_auth.steamclient)
list(APPEND protoFiles steam/steammessages_player.steamclient)
list(APPEND protoFiles steam/steammessages_econ.steamclient)

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

######################################################################

list(APPEND SRCS Boost)

######################################################################

list(APPEND SRCS Main Logging WorkingDir Universe Random Base64 DestructMonitor JobID DataFile Settings AssetKey RateLimit)
list(APPEND SRCS AssetData SendTrade SendInventory PostWithSession AcceptTrade DeclineTrade CancelTrade ExecuteFibers)
list(APPEND SRCS Asio/Asio Asio/Signals Asio/HTTPClient Asio/Fiber Asio/Connections)
list(APPEND SRCS Client/Client Client/Waiter Client/Whiteboard Client/Messageboard Client/Module Client/Sleep Client/ClientInfo)
list(APPEND SRCS Connection/Endpoint Connection/Serialize Connection/Base Connection/TCP Connection/Message Connection/Encrypted)
list(APPEND SRCS OpenSSL/Exception OpenSSL/SHA1 OpenSSL/RSA OpenSSL/AESBase OpenSSL/AES OpenSSL/AESHMAC OpenSSL/Random)
list(APPEND SRCS Steam/OSType Steam/KeyValue Steam/KeyValue_Serialize Steam/KeyValue_Deserialize)
list(APPEND SRCS Steam/MachineInfo Steam/MachineInfo/Linux)
list(APPEND SRCS Web/URLEncode)
list(APPEND SRCS Web/Cookies)
list(APPEND SRCS Web/CookieJar)
list(APPEND SRCS Helpers/Time Helpers/NextSteamDay)
list(APPEND SRCS Helpers/URLs)
list(APPEND SRCS Helpers/HTML)
list(APPEND SRCS Helpers/JSON)
list(APPEND SRCS Helpers/ProtoBuf)
list(APPEND SRCS Helpers/StringCompare)
list(APPEND SRCS UI/UI)
list(APPEND SRCS Modules/AutoLoadTradeoffers)
list(APPEND SRCS Modules/AutoAccept)
list(APPEND SRCS Modules/Executor)
list(APPEND SRCS Modules/Connection)
list(APPEND SRCS Modules/MultiPacket)
list(APPEND SRCS Modules/Heartbeat)
list(APPEND SRCS Modules/UnifiedMessageClient)
list(APPEND SRCS Modules/UnifiedMessageServer)
list(APPEND SRCS Modules/OwnedGames)
list(APPEND SRCS Modules/LicenseList)
list(APPEND SRCS Modules/PackageData)
list(APPEND SRCS Modules/ClientAppList)
list(APPEND SRCS Modules/PersonaState)
list(APPEND SRCS Modules/WebSession)
list(APPEND SRCS Modules/PlayGames)
list(APPEND SRCS Modules/Login-Auth)
list(APPEND SRCS Modules/AddFreeLicense)
list(APPEND SRCS Modules/CardFarmer)
list(APPEND SRCS Modules/DiscoveryQueue)
list(APPEND SRCS Modules/SaleQueue)
list(APPEND SRCS Modules/SaleSticker)
list(APPEND SRCS Modules/ClientNotification)
list(APPEND SRCS Modules/TradeOffers Modules/TradeOffersParser)
list(APPEND SRCS Modules/Inventory)
list(APPEND SRCS Modules/TradeToken)
list(APPEND SRCS Modules/GetBadgeData/ParseBadgePage Modules/GetBadgeData/GetBadgeData Modules/GetBadgeData/Printable)
list(APPEND SRCS WebAPI/WebAPI WebAPI/ISteamDirectory/GetCMList)

include_directories("Christians-HTML-Parser/Headers")

######################################################################

list(TRANSFORM SRCS PREPEND "Sources/")
list(TRANSFORM SRCS APPEND ".cpp")

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${PROTO_SRCS})

# https://stackoverflow.com/questions/13638408/override-compile-flags-for-single-files
SET_SOURCE_FILES_PROPERTIES( ${SRCS} PROPERTIES COMPILE_FLAGS ${WarningOptions} )
