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
