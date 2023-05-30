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
#include <boost/property_tree/ptree.hpp>
#include <boost/fiber/mutex.hpp>

/************************************************************************/
/*
 * This maintains the data file for one account.
 *
 * You can use the examine() call to run a function that gets a
 * const reference to the ptree; your result will be returned
 * from examine().
 *
 * Updates work in a similar way. If your update function returns
 * normally, the tree will be saved back to disk. If it throws,
 * the tree will be reloaded from disk.
 *
 * For now(?), loading/saving is not async.
 */

/************************************************************************/
/*
 * ToDo: when I wrote this for the first time, I was using an older
 * version of boost that didn't have the JSON library yet.
 *
 * I should probably get rid of the rubbish-ish property tree, and
 * just do json instead.
 */

/************************************************************************/

namespace SteamBot
{
	class DataFile
	{
	private:
		static const boost::property_tree::ptree::path_type rootName;

	private:
		const std::filesystem::path filename;
		const std::filesystem::path tempFilename;

		mutable boost::fibers::mutex mutex;
		boost::property_tree::ptree tree;
		bool invalid=false;

	public:
		DataFile(const std::string_view);
		~DataFile();

	private:
		void loadFile();
		void saveFile() const;

	public:
		template <typename FUNC> auto examine(FUNC function) const
		{
            std::lock_guard<decltype(mutex)> lock(mutex);
			assert(!invalid);
			return function(tree.get_child(rootName));
		}

	public:
		void update(std::function<void(decltype(tree)&)>);
	};
}
