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

// **********************************************************************

#include <exception>
#include <typeinfo>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

// **********************************************************************
//
// Call parseMessage() to parse a message.

namespace SteamBot
{
	namespace ProtoBuf
	{
		class InvalidMessageBase : public std::exception
		{
		public:
			virtual const char* what() const noexcept override
			{
				return "invalid protobuf message";
			}
		};

		template <typename T> class InvalidMessage : public InvalidMessageBase
		{
		};

		template <typename T> void parseMessage(boost::asio::const_buffer data, T& result)
		{
			// https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.message_lite
			// https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.io.zero_copy_stream#ZeroCopyInputStream

			google::protobuf::io::ArrayInputStream stream(data.data(), data.size());
			if (!result.ParseFromZeroCopyStream(&stream))
			{
				throw InvalidMessage<T>();
			}
		}
	}
}
