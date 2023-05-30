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
