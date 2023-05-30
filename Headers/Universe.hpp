#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

/************************************************************************/
/*
 * There seem to be different "instances" of Steam, called "universes"
 */

/************************************************************************/
/*
 * The Universe class provides some data for a universe. Use the
 * static get() call to fetch the instance you need.
 */

namespace SteamBot
{
	class Universe
	{
	public:
		/* SteamKit2 calls this 'EUniverse'; values are from SteamLanguage.cs */
		enum class Type : uint32_t
		{
			Invalid = 0,
			Public = 1,
			Beta = 2,
			Internal = 3,
			Dev = 4,
		};

	public:
		const Type type=Type::Invalid;

		typedef std::span<const std::byte, 160> PublicKeyType;
		const PublicKeyType publicKey;

	public:
		static const Universe& get(Type);

	public:
		Universe(Type, const PublicKeyType);

		Universe(const Universe&) =delete;
		Universe& operator=(const Universe&) =delete;
	};
}
