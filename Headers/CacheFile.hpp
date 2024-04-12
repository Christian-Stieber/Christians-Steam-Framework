/*
 * This file is part of "Christians-Steam-Framework"
 * Copyright (C) 2023- Christian Stieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.  If not see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "DataFile.hpp"

/************************************************************************/
/*
 * This provides some aassistance with keeping data in a file
 */

/************************************************************************/

namespace SteamBot
{
    class CacheFile
    {
    public:
        boost::fibers::mutex mutex;
        bool changed=false;
        SteamBot::DataFile* file=nullptr;

    protected:
        std::chrono::steady_clock::time_point lastSave;

    protected:
        CacheFile();
        virtual ~CacheFile();

    protected:
        // You must call this with the filename, before you
        // can use the file
        void init(std::string);

    public:
        void save_noMutex(bool force=true);
        void save(bool force=true);

    public:
        // This will be called during save(). The JSON
        // will have the current file contents; update
        // them (or replace them), and return true.
        // if you return false, DO NOT change anything.
        //
        // If you throw, the file will be reloaded from disk.
        virtual bool updateFile(boost::json::value&) =0;

    public:
        // This called after the file was loaded.
        // The JSON has the file contents; process them
        // as needed.
        virtual void loadedFile(const boost::json::value&);
    };
}
