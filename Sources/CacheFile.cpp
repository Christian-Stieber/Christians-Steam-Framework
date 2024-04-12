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

#include "CacheFile.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

SteamBot::CacheFile::CacheFile() =default;

/************************************************************************/

SteamBot::CacheFile::~CacheFile()
{
    if (changed)
    {
        BOOST_LOG_TRIVIAL(warning) << "CacheFile \"" << file->name << "\" destructed with unsaved data";
    }
}

/************************************************************************/

void SteamBot::CacheFile::init(std::string name)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    assert(file==nullptr);
    file=&SteamBot::DataFile::get(std::move(name), DataFile::FileType::Steam);

    file->examine([this](const boost::json::value& json) {
        loadedFile(json);
    });
}

/************************************************************************/

void SteamBot::CacheFile::save_noMutex(bool force)
{
    assert(file!=nullptr);

    if (changed)
    {
        if (force || decltype(lastSave)::clock::now()-lastSave>=std::chrono::seconds(60))
        {
            file->update([this](boost::json::value& fileData) {
                return updateFile(fileData);
            });
            lastSave=decltype(lastSave)::clock::now();
            changed=false;
        }
    }
}

/************************************************************************/

void SteamBot::CacheFile::save(bool force)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    save_noMutex(force);
}

/************************************************************************/

void SteamBot::CacheFile::loadedFile(const boost::json::value& json)
{
    if (auto object=json.if_object())
    {
        BOOST_LOG_TRIVIAL(info) << "loaded file \"" << file->name << "\" with " << object->size() << " elements";
    }
    else if (auto array=json.if_array())
    {
        BOOST_LOG_TRIVIAL(info) << "loaded file \"" << file->name << "\" with " << array->size() << " elements";
    }
    else
    {
        BOOST_LOG_TRIVIAL(info) << "loaded file \"" << file->name << "\"";
    }
}
