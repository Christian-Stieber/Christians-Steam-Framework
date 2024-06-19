######################################################################
#
# Use with a command like
#   cmake -S . -B build/Debug -D CMAKE_BUILD_TYPE=Debug
#   cmake --build build/Debug

# set(CMAKE_VERBOSE_MAKEFILE ON)

######################################################################

cmake_minimum_required(VERSION 3.25)

project(ChristiansSteamFramework LANGUAGES CXX)
add_library(${PROJECT_NAME} STATIC)

target_include_directories(${PROJECT_NAME} PUBLIC "Headers")
target_include_directories(${PROJECT_NAME} PUBLIC "Christians-HTML-Parser/Headers")

######################################################################

include("CompileOptions.cmake")
setCompileOptions(${PROJECT_NAME})

######################################################################

add_subdirectory(steamdatabase)
target_link_libraries(${PROJECT_NAME} PUBLIC steamdatabase-protobufs)

######################################################################

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL 3 REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenSSL::SSL OpenSSL::Crypto)

######################################################################
#
# Debian only has 1.81, so we have to deal with the boost::url stuff
# until we get an update there

set(Boost_USE_STATIC_LIBS ON)
list(APPEND BoostComponents system system log log_setup iostreams fiber program_options)
list(APPEND BoostOptionalComponents url)
find_package(Boost 1.81.0 REQUIRED COMPONENTS ${BoostComponents} OPTIONAL_COMPONENTS ${BoostOptionalComponents})
target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})

######################################################################

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(${PROJECT_NAME} PUBLIC pthread systemd atomic)
endif()

######################################################################

function(addSource directory)
  list(TRANSFORM ARGN PREPEND "Sources/${directory}/")
  list(TRANSFORM ARGN APPEND ".cpp")
  target_sources(${PROJECT_NAME} PRIVATE ${ARGN})
endfunction(addSource)

addSource("Helpers" Time NextSteamDay URLs HTML JSON ProtoBuf NumberString StringCompare HexString PrintSize)

addSource("Modules"
  AutoLoadTradeoffers AutoAccept Executor Connection MultiPacket Heartbeat UnifiedMessageClient
  UnifiedMessageServer OwnedGames LicenseList PackageInfo PackageData ClientAppList PersonaState
  WebSession PlayGames Login-Auth AddFreeLicense CardFarmer DiscoveryQueue SaleQueue ViewStream
  SaleSticker ClientNotification TradeOffers TradeOffersParser Inventory TradeToken LoginTracking
  InventoryNotification BadgeData/GetBadgeData BadgeData/BadgeInfo Login-Session)

addSource("Settings"
    Settings SettingBool SettingBotName SettingString)

addSource("."
  Main Logging WorkingDir Universe Random Base64 DestructMonitor JobID DataFile AssetKey
  RateLimit Exception AssetData SendTrade SendInventory PostWithSession AcceptTrade DeclineTrade
  CancelTrade ExecuteFibers MaintainBPE CacheFile AppInfo Boost ParseToken)

addSource("Asio" Asio Signals HTTPClient Fiber Connections)
addSource("Client" Client Waiter Whiteboard Messageboard Execute Module Sleep ClientInfo)
addSource("Connection" Endpoint Serialize Base TCP Message Encrypted)
addSource("OpenSSL" Exception SHA1 RSA AESBase AES AESHMAC Random)
addSource("Web" URLEncode Cookies CookieJar)
addSource("WebAPI" WebAPI ISteamDirectory/GetCMList)
addSource("Cloud" Apps Files Platform)
addSource("UI" UI)

addSource("Steam"
  OSType KeyValue KeyValue_Serialize KeyValue_Deserialize KeyValue_Deserialize_Text MachineInfo
  MachineInfo/Linux MachineInfo/Windows)
