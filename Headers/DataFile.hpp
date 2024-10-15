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

#include <filesystem>
#include <boost/json.hpp>
#include <boost/fiber/mutex.hpp>

/************************************************************************/
/*
 * This maintains datafiles. While initially intended to hold
 * the data for one account (like login-keys), I don't see
 * why this couldn't be used for other stuff.
 *
 * You can use the examine() call to run a function that gets a
 * const reference to the json; your result will be returned
 * from examine().
 *
 * Updates work in a similar way. If your update function returns
 * true, the json will be saved back to disk. If it throws, the
 * tree will be reloaded from disk. If it returnd false, make sure
 * you really don't change anything.
 *
 * For now(?), loading/saving is not async.
 */

/************************************************************************/

namespace SteamBot
{
	class DataFile
	{
    public:
        enum class FileType { Account, Steam };

	public:
        const FileType fileType;
        std::string name;

    private:
		const std::filesystem::path filename;
		const std::filesystem::path tempFilename;

		mutable boost::fibers::mutex mutex;
        boost::json::value json;
		bool invalid=false;

    private:
		DataFile(std::string&&, FileType);

    public:
		~DataFile();

	private:
		void loadFile();
		void saveFile() const;

	public:
		template <typename FUNC> auto examine(FUNC function) const
		{
            std::lock_guard<decltype(mutex)> lock(mutex);
			assert(!invalid);
			return function(json);
		}

	public:
		void update(std::function<bool(boost::json::value&)>);

    public:
        static DataFile& get(std::string_view, FileType);
	};
}
