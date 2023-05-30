# WARNING

This IS heavily work-in-progress. Chances are there's nothing to see for you here.

# GOALS

This project, more or less, started with me having some gripes with a much more
famous idling bot based on SteamKit, so I figured I'd just make my own.

Currently, the main goals are:

* making a C++ framework to write Steam bots, similar to projects such as SteamKit
* making a bot to idle cards
* making the bot create trades for cards
* while everything is in one source tree right now, I fully intend to separate the
actual "bot" from the "framework" eventually.

As for the time beyond, I don't know yet. This is a one-man spare-time project,
so I'm trying to stick to a reasonably realistic plan.

Note that while there's some intention somewhere in the back of my
head to allow for other types of UI eventually, current development
focuses on a simple console-based CLI.

# "CURRENT" STATE

I don't know how "current" the "current" really is; maybe I'll update
it when something interesting happens :-)

* lots of basic infrastructure to build on
* runs a single client instance
* supports SteamGuard codes for login
* unconditionally runs a single hardcoded game

As I said, it's work in progress -- and the above is certainly useful
progress towards the final goal.

# PLATFORMS

If, for whatever reaason, you are stupid enough to want to try this:
while the code is meant to be portable, there are a couple of
platform-specific items such as getting information about the current
machine. Code might not exist yet for all platforms, and if it does
exist then it certainly hasn't been tested.

Currently, I'm developing this on a Debian Linux box. Windows is high
on the priority list (actually, that Debian runs in a VM on a Windows
box...), but I haven't done it yet.

MacOS is likely to not ever happen, due to lack of hardware on my side.

# DEPENDENCIES

* C++20 compiler
* boost 1.81
* protobuf, ssl, and a couple other minor libraries
* I'm not very happy with the cmake stuff, but I'm currently using that to build
