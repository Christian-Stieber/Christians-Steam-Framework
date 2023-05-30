#pragma once

#include <string>
#include <vector>
#include <span>

/************************************************************************/

namespace SteamBot
{
	namespace Base64
	{
		std::string encode(std::span<const std::byte>);
		std::vector<std::byte> decode(const std::string&);
	}
}
