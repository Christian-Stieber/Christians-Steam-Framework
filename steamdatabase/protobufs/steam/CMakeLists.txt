set(myDir "steam")

list(APPEND protoFiles
  steammessages_base
  steammessages_clientserver
  steammessages_clientserver_login
  steammessages_clientserver_2
  steammessages_clientserver_appinfo
  enums
  encrypted_app_ticket
  steammessages_unified_base.steamclient
  steammessages_player.steamclient
  steammessages_clientserver_uds
  steammessages_clientserver_friends
  steammessages_auth.steamclient
  steammessages_player.steamclient
  steammessages_econ.steamclient
  enums_productinfo
)

######################################################################

include("../../SubDir.cmake")
