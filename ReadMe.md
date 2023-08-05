# WARNING

This IS heavily work-in-progress. Chances are there's nothing to see for you here.

# GOALS

This project, more or less, started with me having some gripes with a much more
famous idling bot based on SteamKit, so I figured I'd just make my own.

Currently, the main goals are:

* making a C++ framework to write Steam bots, similar to projects such as SteamKit
* making a bot to idle cards
* making the bot create trades for cards

As for the time beyond, I don't know yet. This is a one-man spare-time project, so I'm trying to stick to a reasonably realistic plan.

Note that the bot is in a separate repository: https://github.com/Christian-Stieber/Christians-Steam-Bot

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

# FEATURES

Again, this list might not be complete/current. But, some things that are there include:
* multiple clients running concurrently, clients are based around internal asynchronicity as well
* modular setup
* for now, only TCP connections are supported.\
  I guessed it might the easiest of the three, and wanted to get other things done
* Login:
    * password-based
    * Steamguard EMail, Steamguard Device, Device confirmation.\
      Note that you can't select this; it will prefer device confirmations if they are offered
* sending/receiving Steam client messages.\
  Many of those do not have specific support, though.\
  Protobuf messages in particular (and I've rarely encountered any that aren't) are very easy to work with.
* some higher level support for Steam client functionality, such as
     * getting lists of licenses or owned games
     * registering a free license
     * "playing" games
* builtin HTTP support:
     * an HTTP client (light wrapper around `boost::beast` and integration with the framework)
     * support for "authenticated" webpage-queries to Steam
     * a simple HTML parser (can parse the Steam badge page, chokes on the Amazon homepage)
* module to get badge data
     * there's also a "fake" farming module that merely lists games that have card drops remaining.\
       I'm calling that a starting point, as the thing is still work-in-progress
* support for UnifiedSteamMessages:
     * client-initiated\
       Only a "blocking" API right now -- but note that this only blocks the fiber making the call, nothing else.
     * server-initiated (notifications in particular) are high up on my ToDo-list

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
