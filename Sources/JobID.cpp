#include "JobID.hpp"

#include <atomic>
#include <chrono>

/************************************************************************/

static std::atomic<SteamBot::JobID::valueType> sequence{0};

typedef std::chrono::system_clock SystemClock;
static const auto startTime=SystemClock::to_time_t(SystemClock::now());

/************************************************************************/

SteamBot::JobID::JobID()
{
	setSequentialCount(sequence++);
	setStartTime(startTime);
}
