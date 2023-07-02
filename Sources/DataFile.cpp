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

#include "DataFile.hpp"
#include "EnumString.hpp"

#include <boost/log/trivial.hpp>
#include <fstream>
#include <sstream>

/************************************************************************/
/*
 * Note that Steam account names can only have a-z, A-Z, 0-9 or _ as
 * characters, so we can use them as filenames.
 *
 * I'm just applying the same rule to the other filetypes as well.
 */

static std::filesystem::path makeFilename(const std::string_view& name, SteamBot::DataFile::FileType fileType)
{
	for (const char c : name)
	{
		assert((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_');
	}

	std::string result;
    switch(fileType)
    {
    case SteamBot::DataFile::FileType::Account:
        result+="Account-";
        break;

    case SteamBot::DataFile::FileType::Steam:
        result+="Steam-";
        break;

    default:
        assert(false);
    }
	result+=name;
	result+=".json";
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
/*
 * This does NOT lock the mutex.
 */

void SteamBot::DataFile::loadFile()
{
    json=boost::json::object();
	invalid=true;

	const auto status=std::filesystem::status(filename);
	switch(status.type())
	{
	case std::filesystem::file_type::not_found:
		invalid=false;
		break;

	case std::filesystem::file_type::regular:
        {
            BOOST_LOG_TRIVIAL(info) << "reading data file " << filename;

            std::stringstream data;
            data << std::ifstream(filename).rdbuf();

            /* ToDo: add decryption */

            json=boost::json::parse(data.view()).as_object();
        }
		invalid=false;
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

    std::stringstream data;
    data << json;

    /* ToDo: add encryption */

    {
        std::ofstream output(tempFilename, std::ios_base::out | std::ios_base::trunc);
        output << data.rdbuf();
    }

	std::filesystem::rename(tempFilename, filename);
	BOOST_LOG_TRIVIAL(info) << "updated data file " << filename;
}

/************************************************************************/
/*
 * "name" needs to be unique within its FileType.
 */

SteamBot::DataFile::DataFile(std::string_view name, SteamBot::DataFile::FileType fileType_)
	: fileType(fileType_),
      filename(makeFilename(name, fileType)),
	  tempFilename(makeTempFilename(filename))
{
	loadFile();
}

/************************************************************************/

void SteamBot::DataFile::update(std::function<void(boost::json::value&)> function)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
	assert(!invalid);
	try
	{
		function(json);
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
