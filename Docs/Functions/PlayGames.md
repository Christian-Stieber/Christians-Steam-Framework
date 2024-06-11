# Idling games

The `PlayGames` module let's you idle games, by just telling Steam we're playing it.

## `SteamBot::Modules::PlayGames::Messageboard::PlayGames`

While you can send this message to start/stop games, you probably use the `play' convience function
to do the same: start or stop a list of games.

## `SteamBot::Modules::PlayGames::Whiteboard::PlayingGames`

This is a whiteboard item that tells us what games we are idling,

## Notes

There are a few things to note about this functionality:

* We stop playing a game every 10 minutes for 5 seconds; this is to encourage Steam to update its
  playtime data and do other things like dropping cards. This is a hidden feature -- it's not reflected
  in the API
* I don't currently know how get information from Steam about what games they actually consider being
  played, so this never reflects "real" data: `PlayingGames` only tells you what the module thinks
  it's playing
* Along the same lines, I believe Steam has an upper limit on the number of games played concurrrently.
  Nothing is hardcoded into the module (for now?), though -- so, keep the number reasonably lowish.
* There is no tracking of how many requests have been made to play a certain game, or who made these
  requests. If multiple modules decide to play a game, and one stops it, it stops.

