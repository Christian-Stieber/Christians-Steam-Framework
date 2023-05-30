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

#include "Config.hpp"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string/case_conv.hpp>

/************************************************************************/

static const std::string filename="Config.xml";

/************************************************************************/

static const boost::property_tree::ptree& getConfig()
{
	static const boost::property_tree::ptree config=[](){
		boost::property_tree::ptree tree;
		boost::property_tree::read_xml(filename, tree);
		return tree;
	}();

	return config;
}

/************************************************************************/

static std::string getLowerCase(const std::string& name)
{
	std::string result=getConfig().get<std::string>(name);
	boost::algorithm::to_lower(result);
	return result;
}

/************************************************************************/

SteamBot::Config::SteamAccount::SteamAccount()
	: user(getLowerCase("steam-account.user")),
	  password(getConfig().get<std::string>("steam-account.password"))
{
	BOOST_LOG_TRIVIAL(info) << "using steam account \"" << user << "\"";
}

/************************************************************************/

SteamBot::Config::SteamAccount::~SteamAccount() =default;

/************************************************************************/

const SteamBot::Config::SteamAccount& SteamBot::Config::SteamAccount::get()
{
	static const SteamAccount account;
	return account;
}
