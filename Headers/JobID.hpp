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

#include "GlobalID.hpp"

/************************************************************************/
/*
 * Default constructor will set a unique job id
 */

namespace SteamBot
{
	class JobID : public GlobalID
	{
		using GlobalID::GlobalID;

	public:
		JobID();
	};
}

/************************************************************************/

template <> struct std::hash<SteamBot::JobID>
{
    std::size_t operator()(const SteamBot::JobID& jobId) const noexcept
    {
		return std::hash<SteamBot::JobID::bitfieldType>{}(jobId);
    }
};
