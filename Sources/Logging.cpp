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

#include "Logging.hpp"

#include <boost/log/core.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/trivial.hpp>

/************************************************************************/

void SteamBot::Logging::init()
{
    namespace keywords=boost::log::keywords;
    boost::log::add_file_log
    (
        keywords::file_name = "Framework.log",
        keywords::target_file_name = "Framework_%N.log",
        keywords::open_mode = std::ios_base::out | std::ios_base::app,
        keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
        keywords::format = "[%TimeStamp%]: %Message%",
        keywords::auto_flush = true,
        keywords::enable_final_rotation = false
    );

    boost::log::add_common_attributes();

    BOOST_LOG_TRIVIAL(info) << "============================== program launch ==============================";
}
