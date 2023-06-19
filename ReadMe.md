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

# PLATFORMS

If, for whatever reaason, you are stupid enough to want to try this:
while the code is meant to be portable, there are a couple of
platform-specific items such as getting information about the current
machine. Code might not exist yet for all platforms, and if it does
exist then it certainly hasn't been tested.

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

I'm now working on a project for Visual Studio 2022. This is still work-in-progress.
Currently, it uses boch nuget and vcpkg package manegers; make sure you install vcpkg from the Visual Studio tool selection, and run `vcpkg integrate install` in the Visual Studio command prompt.

I think this should do it.

# USAGE

## Using

For now (and the forseeable future) there's only the console UI, and its currently a very basic implementation. Launch the program in a command prompt/terminal window.

I've only recently added the UI, so outputs might be a bit scarce -- but I don't want to spam the window with clutter either.

You can hit the `Return/Enter` or `TAB` key at any time (when there's no input prompt) to enter command mode. There aren't a whole lot of commands yet, but the basic infrastructure works so I'll be adding more as I go along.

The `help` command gives a list of commands. Note that they are case-sensitive, so the `EXIT` command really needs to be entered like that.

Just hit return to enter an empty command line to exit command mode.

Keep in mind that all other console activity will be blocked while in command mode. In particular, if you `launch`/`create` a bot, and it prompts for a password or a SteamGuard code, you will not see this prompt until you leave command mode.

## Files

The bot stores data in `%LOCALAPPDATA%\Christian-Stiebeer\Steam-framework` or `~/.SteamBot/`. For now, these are unencrypted, but I'll change that eventually.

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
