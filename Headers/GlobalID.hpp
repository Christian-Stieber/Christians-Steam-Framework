#pragma once

#include "BitField.hpp"

#include <ctime>

/************************************************************************/

namespace SteamBot
{
	class GlobalID : public BitField<uint64_t>
	{
		using BitField::BitField;

		STEAM_BITFIELD_MAKE_ACCESSORS(uint32_t, SequentialCount, 0, 20);
		STEAM_BITFIELD_MAKE_ACCESSORS(uint32_t, StartTimeSeconds, 20, 30);
		STEAM_BITFIELD_MAKE_ACCESSORS(uint8_t, ProcessID, 50, 4);
		STEAM_BITFIELD_MAKE_ACCESSORS(uint16_t, BoxID, 54, 11);

	public:
		/* C++ doesn't have a timegm(), as far as I can tell, so let's just
		 * assume our epoch is as expected and hardcode the result of running
		 *    date -u -d "2005-01-01 00:00:00" +"%s"
		 */

		inline static constexpr std::time_t timeOffset=1104537600;

		void setStartTime(std::time_t time)
		{
			setStartTimeSeconds(time-timeOffset);
		}

		std::time_t getStartTime() const
		{
			return getStartTimeSeconds()+timeOffset;
		}
	};
}
