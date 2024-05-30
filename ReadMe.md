# WARNING

This IS heavily work-in-progress. Chances are there's nothing to see for you here.

# GOALS

This project, more or less, started with me having some gripes with a much more
famous idling bot based on SteamKit, so I figured I'd just make my own.

Currently, the main goals are to make a framework to "deal with Steam" in various
way, particularly stuff that I want to automate for myself. Currently, higher
level features include (this list might not be up to date):

* card farming
* stream "viewing" (to get drops; but it doesn't stop automatically)
* sale queues
* daily sale stickers
* trading the entire (tradable) inventory to another account
* accepting trades
* listing owned games
* listing inventory

There's low level stuff, of course:

* making and using a client connection (i.e. protobufs)
* logins
* making authenticated https queries

so in theory you can make your own features if you wanted to.

This is a one-man spare-time project, so progress is slow and mostly based on what I want to do.

Note that a bot is in a separate repository: https://github.com/Christian-Stieber/Christians-Steam-Bot

# PLATFORMS

If, for whatever reaason, you are stupid enough to want to try this: while the code is meant to be portable, there are a couple of platform-specific items such as getting information about the current machine. Code might not exist yet for all platforms, and if it does exist then it certainly hasn't been tested.

## Linux

Currently, I'm developing this on a Debian Linux box.

This list of required packages might be incomplete:

* `g++` (at least version 12)
* `libboost-all-dev` (at least 1.81)
* `cmake`
* `libprotobuf-dev`
* `protobuf-compiler`
* `libssl-dev`
* `libsystemd-dev`
* `libzstd-dev`
* `libbz2-dev`
* `liblzma-dev`

## Windows

I'm now maintaining a project for Visual Studio 2022 -- the free edition, whatever it might be called today. It might be a bit behind, but I generally try to update it regularly.

Currently, it uses boch nuget and vcpkg package manegers; make sure you install vcpkg from the Visual Studio tool selection, and run `vcpkg integrate install` in the Visual Studio command prompt.

I think this should do it.

# DATA FILES

The bot stores data in `%LOCALAPPDATA%\Christian-Stieber\Steam-framework` or `~/.Christians-Steam-Framework`. For now, these are unencrypted, but I'll change that eventually.

You'll find a logfile there as well -- but I'm not sure whether it's properly flushed on Windows.

# 3RD PARTY CODE

In addition to libraries that are used through packages, the framework includes some other 3rd party code in its source tree:

* https://github.com/steamdatabase/protobufs.git \
  Was added as a **submodule**
* https://github.com/Neargye/magic_enum \
  Code has been **copied** into my source tree. \
  See Headers/External/MagicEnum.
* https://github.com/boostorg/fiber/tree/develop/examples/asio \
  Boost fiber has no official asio support (or asio has no fiber support,
  whatever you prefer). \
  Thus, I've **copied** the asio code from the fiber documentation and adapted
  it a bit to make it work. \
  See Headers/boost.

# INSPIRATIONS

"Inspirations" on how the Steam stuff works was taken from \
   https://github.com/SteamRE/SteamKit \
   https://github.com/JustArchiNET/ArchiSteamFarm

SteamKit in particular has also provided me with lists of
enum values to copy-paste.
