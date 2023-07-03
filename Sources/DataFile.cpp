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

typedef SteamBot::DataFile DataFile;

/************************************************************************/
/*
 * Note that Steam account names can only have a-z, A-Z, 0-9 or _ as
 * characters, so we can use them as filenames.
 *
 * I'm just applying the same rule to the other filetypes as well.
 */

static std::filesystem::path makeFilename(const std::string& name, DataFile::FileType fileType)
{
	for (const char c : name)
	{
		assert((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_');
	}

	std::string result;
    switch(fileType)
    {
    case DataFile::FileType::Account:
        result+="Account-";
        break;

    case DataFile::FileType::Steam:
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

void DataFile::loadFile()
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

void DataFile::saveFile() const
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

DataFile::DataFile(std::string&& name_, DataFile::FileType fileType_)
	: fileType(fileType_),
      name(std::move(name_)),
      filename(makeFilename(name, fileType)),
	  tempFilename(makeTempFilename(filename))
{
	loadFile();
}

/************************************************************************/
/*
 * Note: this only saves if you return true -- be sure to not
 * change anyting if you return false!
 */

void DataFile::update(std::function<bool(boost::json::value&)> function)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
	assert(!invalid);
	try
	{
		if (function(json))
        {
            saveFile();
        }
	}
	catch(...)
	{
		loadFile();
		throw;
	}
}

/************************************************************************/

DataFile::~DataFile() =default;

/************************************************************************/

DataFile& DataFile::get(std::string_view name, DataFile::FileType type)
{
    class Files
    {
    private:
        boost::fibers::mutex mutex;
        std::vector<std::unique_ptr<DataFile>> files;

    public:
        DataFile& get(std::string_view name, DataFile::FileType type)
        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            for (const auto& file : files)
            {
                if (file->fileType==type && file->name==name)
                {
                    return *file;
                }
            }
            {
                std::unique_ptr<DataFile> file(new DataFile(std::string(name), type));
                files.emplace_back(std::move(file));
            }
            return *files.back();
        }
    };

    static auto& files=*new Files;
    return files.get(name, type);
}
