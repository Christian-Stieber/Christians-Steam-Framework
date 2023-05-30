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

#include <charconv>
#include <cassert>

#include "SteamID.hpp"

/************************************************************************/

static void addNumber(std::string& result, uint32_t value)
{
	char buffer[32];
	auto t=std::to_chars(buffer, buffer+sizeof(buffer), value);
	assert(t.ec==std::errc());
	result.append(buffer, t.ptr);
}

/************************************************************************/

std::string Steam::SteamID::steam2String() const
{
	switch(getAccountType())
	{
	case AccountType::Invalid:
	case AccountType::Individual:
		{
			std::string result;
			result+="STEAM_";
			auto universe=getUniverseType();
			addNumber(result, (universe<=Universe::Type::Public) ? 0 : toInteger(universe));
			result+=':';
			auto accountId=getAccountId();
			addNumber(result, accountId & 1);
			result+=':';
			addNumber(result, accountId >> 1);
			return result;
		}

	default:
		return steam3String();
	}
	assert(false);
}

/************************************************************************/

std::string Steam::SteamID::steam3String() const
{
#warning ToDo
	assert(false);
}
