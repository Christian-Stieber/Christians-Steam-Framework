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
