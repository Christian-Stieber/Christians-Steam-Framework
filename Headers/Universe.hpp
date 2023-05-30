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
