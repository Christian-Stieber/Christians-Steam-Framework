# Game information

On Steam, games are identified by the `SteamBot::AppID` enumeration.

## `SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames`

Essentially, this provides a table of all games owned by the account, indexed by `SteamBot::AppID`.

Individual `GameInfo` items currently give you
* the name of the game
* the accumulated playtime
* the time when it was last played
  
More data might be added in the feature -- a lot of what's available is currently ignored,
since I didn't need it.

You can get the data from the whiteboard, or through a convenience call that looks up the `AppID`
and returns the `GameInfo`, if we have one. It will generally be updated automatically when new
license data is received from Steam.

There is an additional `std::string getName(AppID)` convenience call that also supports names for
some special AppIDs. As an example, in some contexts, the Steam client itself has an `AppID` 7;
while there is no game info for this `AppID`, the `getName()` call will still return `Steam Client`
from an internal list of hardcoded special names.

## `SteamBot::Modules::OwnedGames::Messageboard::GameChanged`

This is a message that will be sent on the messageboard if individual games change. There will
be one message per changed game, and it will currently be posted under these circumstances:
* a new game was added to the table
* data for a game was updated, usually in respone to an `UpdateGames` request

Thus, don't expect this to be a live-tracker of everything that might change.

`GameChanged` messages are explicitly NOT sent on the initial construction of the table, to avoid
flooding the client with possibly thousands of `GameChanged` messages that are not even actual
changes. This is strictly a mechanism to report changes that happen after that.

## `SteamBot::Modules::OwnedGames::Messageboard::UpdateGames`

This is a message that you can send TO the `OwnedGames` module, to request a re-read of the
game data for specific `AppID`s. This is currently used by the `PlayGames` module to effect
an update of the last-played and accumulated playtime data.
