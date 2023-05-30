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
