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

#include <string>

#include "BitField.hpp"
#include "Steam/AccountType.hpp"
#include "Universe.hpp"

/************************************************************************/

namespace SteamBot
{
	class SteamID;
}

/************************************************************************/
/*
 * SteamKit2, SteamID.cs, SteamID
 *
 * This 64-bit structure is used for identifying various objects on
 * the Steam network.
 *
 * Note: while C++ has bitfields, they are too implemented-defined to
 * be really useful. Thus, I'm going with the same BitVector thing
 * that SteamKit2 uses.
 */

class SteamBot::SteamID : public SteamBot::BitField<uint64_t>
{
	using BitField::BitField;

private:
	typedef Accessor<0, 32, uint32_t> AccountIdField;
	typedef Accessor<52, 4, Steam::AccountType> AccountTypeField;
	typedef Accessor<56, 8, Universe::Type> UniverseField;

	/*
	 * I'm not sure yet what this is -- it appears it can be a lot of
	 * things, depending on other parameters.
	 */
	typedef Accessor<32, 20, uint32_t> AccountInstanceField;

public:
	inline void setAccountId(AccountIdField::valueType accountId)
	{
		AccountIdField::set(*this, accountId);
	}

	inline AccountIdField::valueType getAccountId() const
	{
		return AccountIdField::get(*this);
	}

public:
	inline void setAccountType(Steam::AccountType accountType)
	{
		AccountTypeField::set(*this, accountType);
	}

	inline Steam::AccountType getAccountType() const
	{
		return AccountTypeField::get(*this);
	}

public:
	inline void setUniverseType(Universe::Type universe)
	{
		UniverseField::set(*this, universe);
	}

	inline Universe::Type getUniverseType() const
	{
		return UniverseField::get(*this);
	}

	inline void setUniverse(const Universe& universe)
	{
		setUniverseType(universe.type);
	}

	inline const Universe& getUniverse() const
	{
		return Universe::get(getUniverseType());
	}

public:
	inline void setAccountInstance(AccountInstanceField::valueType accountInstance)
	{
		AccountInstanceField::set(*this, accountInstance);
	}

	inline AccountInstanceField::valueType getAccountInstance() const
	{
		return AccountInstanceField::get(*this);
	}

public:
	std::string steam2String() const;
	std::string steam3String() const;
};
