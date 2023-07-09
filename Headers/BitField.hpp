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

#include <concepts>
#include <cstdint>
#include <functional>

/************************************************************************/
/*
 * This was inspired by SteamKit2 BitVector64 (SteamID.cs)
 *
 * This also means I'm numbering bits the same way -- offser 0 is the
 * lowest/rightmost bit.
 *
 * Note: even though C++ has bitfields, they are too
 * implementation-defined to be actually useful.
 */

/************************************************************************/
/*
 * You are expected to derive your own class from this so you can wrap
 * the offsets and counts into your own set/get functions.
 */

namespace SteamBot
{
	template <typename T> requires std::is_unsigned_v<T> class BitField
	{
	public:
		typedef T valueType;
		typedef BitField bitfieldType;

	private:
	    T data=0;

	public:
		explicit operator T() const
		{
			return data;
		}

		T getValue() const
		{
			return data;
		}

        void setValue(T newValue)
        {
            data=newValue;
        }

	public:
		BitField(T myData=0)
			: data(myData)
		{
		}

	private:
		static inline constexpr T valueMask(uint8_t count)
		{
			return (1<<count)-1;
		}

	private:
		inline T get(uint8_t offset, uint8_t count) const
		{
			return (data>>offset) & valueMask(count);
		}

		inline void set(uint8_t offset, uint8_t count, T value)
		{
			const auto mask=valueMask(count);
			data=(data & ~( mask<<offset)) | ((value & mask)<<offset);
		}

	protected:
		template <uint8_t offset, uint8_t count, typename U> class Accessor
		{
		public:
			typedef U valueType;

		public:
			inline static void set(BitField& bitField, U value)
			{
				bitField.set(offset, count, static_cast<T>(value));
			}

			inline static U get(const BitField& bitField)
			{
				return static_cast<U>(bitField.get(offset, count));
			}
		};

	public:
		friend bool operator==(const BitField& left, const BitField& right)
		{
			return left.data==right.data;
		}
	};
}

/************************************************************************/

// https://stackoverflow.com/questions/4189945/templated-class-specialization-where-template-argument-is-a-template
template <typename T> struct std::hash<SteamBot::BitField<T>>
{
    std::size_t operator()(const SteamBot::BitField<T>& bitfield) const noexcept
    {
		return std::hash<T>{}(bitfield.getValue());
    }
};

/************************************************************************/
/*
 * Macro to produce an accessor typedef to define the field, and
 * set/get member functions.
 */

#define STEAM_BITFIELD_MAKE_ACCESSORS(type, fieldName, offset, size) \
	public: \
	typedef Accessor<offset, size, type> fieldName##Field; \
	void set##fieldName(fieldName##Field::valueType value) { fieldName##Field::set(*this, value); } \
	fieldName##Field::valueType get##fieldName() const { return fieldName##Field::get(*this); } \
	static_assert(true, "so we can put a semicolon after the macro")
