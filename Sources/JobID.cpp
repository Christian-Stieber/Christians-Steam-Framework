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

#include "JobID.hpp"

#include <atomic>
#include <chrono>

/************************************************************************/

static std::atomic<SteamBot::JobID::SequentialCountField::valueType> sequence{0};

static const auto startTime=SteamBot::GlobalID::Clock::now();

/************************************************************************/

SteamBot::JobID::JobID()
{
	setSequentialCount(++sequence);
	setStartTime(startTime);
}
