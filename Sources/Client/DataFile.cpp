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

#include "Client/DataFile.hpp"
#include "EnumString.hpp"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * For some reason, property trees don't force a root node --
 * the XML writer will happily write invalid XML...
 *
 * The APIs don't export our root node, so you don't need to
 * worry about it.
 */

const boost::property_tree::ptree::path_type SteamBot::DataFile::rootName="data";

/************************************************************************/
/*
 * Note that Steam account names can only have a-z, A-Z, 0-9 or _ as
 * characters, so we can use them as filenames.
 */

static std::filesystem::path makeFilename(const std::string_view& accountName)
{
	for (const char c : accountName)
	{
		assert((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_');
	}

	std::string result;
	result+="Data-";
	result+=accountName;
	result+=".xml";
	return std::filesystem::absolute(result);
}

/************************************************************************/

static std::filesystem::path makeTempFilename(const std::filesystem::path& filename)
{
	std::filesystem::path result=filename.stem();
	result+="-new";
	result+=filename.extension();
	return std::filesystem::absolute(result);
}

/************************************************************************/

#if 0
static void Print(const boost::property_tree::ptree& tree, std::string indent="")
{
	for (auto& node : tree)
	{
		BOOST_LOG_TRIVIAL(debug) << indent << node.first << ": " << node.second.get_value<std::string>();
		Print(node.second, indent+"   ");
	}
}
#endif

/************************************************************************/
/*
 * Note: I don't want to use trim_whitespace, since it also affects
 * data items. However, not doing anything will add more and more
 * whitespace with each iteration of read/write, so I need to do
 * something.
 * That something is deleting values from nodes that have children;
 * our XML doesn't do that anyway.
 */

static void repairTree(boost::property_tree::ptree& tree)
{
	for (auto& node : tree)
	{
		if (!node.second.empty())
		{
			node.second.put_value("");
			repairTree(node.second);
		}
	}
}

/************************************************************************/
/*
 * This does NOT lock the mutex.
 */

void SteamBot::DataFile::loadFile()
{
	tree.clear();
	invalid=true;

	const auto status=std::filesystem::status(filename);
	switch(status.type())
	{
	case std::filesystem::file_type::not_found:
		{
			boost::property_tree::ptree child;
			tree.put_child(rootName,child);
		}
		invalid=false;
		break;

	case std::filesystem::file_type::regular:
		BOOST_LOG_TRIVIAL(info) << "reading data file " << filename;
		boost::property_tree::read_xml(filename, tree);
		invalid=false;
		repairTree(tree);
		break;

	default:
		BOOST_LOG_TRIVIAL(error) << filename << " is of unsupported type \"" << enumToStringAlways(status.type()) << "\"";
		throw std::runtime_error("unsupported file type");
	}
}

/************************************************************************/
/*
 * This does NOT lock the mutex.
 */

void SteamBot::DataFile::saveFile() const
{
	assert(!invalid);
	boost::property_tree::write_xml(tempFilename, tree, std::locale(), boost::property_tree::xml_writer_make_settings<std::string>('\t', 1));
	std::filesystem::rename(tempFilename, filename);
	BOOST_LOG_TRIVIAL(info) << "updated data file " << filename;
}

/************************************************************************/

SteamBot::DataFile::DataFile(const std::string_view accountName)
	: filename(makeFilename(accountName)),
	  tempFilename(makeTempFilename(filename))
{
	loadFile();
}

/************************************************************************/

void SteamBot::DataFile::update(std::function<void(decltype(tree)&)> function)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
	assert(!invalid);
	try
	{
		function(tree.get_child(rootName));
		saveFile();
	}
	catch(...)
	{
		loadFile();
		throw;
	}
}

/************************************************************************/

SteamBot::DataFile::~DataFile() =default;
