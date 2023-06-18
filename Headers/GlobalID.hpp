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

#include "BitField.hpp"

#include <ctime>
#include <chrono>

/************************************************************************/

namespace SteamBot
{
	class GlobalID : public BitField<uint64_t>
	{
		using BitField::BitField;

		STEAM_BITFIELD_MAKE_ACCESSORS(uint32_t, SequentialCount, 0, 20);
		STEAM_BITFIELD_MAKE_ACCESSORS(uint32_t, StartTimeSeconds, 20, 30);
		STEAM_BITFIELD_MAKE_ACCESSORS(uint8_t, ProcessID, 50, 4);
		STEAM_BITFIELD_MAKE_ACCESSORS(uint16_t, BoxID, 54, 10);

    public:
        typedef std::chrono::system_clock Clock;

	public:
		/* C++ doesn't have a timegm(), as far as I can tell, so let's just
		 * assume our epoch is as expected and hardcode the result of running
		 *    date -u -d "2005-01-01 00:00:00" +"%s"
		 */

		inline static const Clock::time_point epoch2005=Clock::from_time_t(1104537600);

		void setStartTime(Clock::time_point time)
		{
			setStartTimeSeconds(static_cast<StartTimeSecondsField::valueType>(std::chrono::duration_cast<std::chrono::seconds>(time-epoch2005).count()));
		}

        Clock::time_point getStartTime() const
		{
            return epoch2005+std::chrono::seconds(getStartTimeSeconds());
		}
	};
}
