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

#include <span>
#include <cstddef>
#include <utility>
#include <ranges>
#include <concepts>

#include <boost/endian/conversion.hpp>

#include <google/protobuf/message_lite.h>

#include "Connection/Base.hpp"
#include "Helpers/ProtoPuf.hpp"

#include <boost/log/trivial.hpp>

#include "TypeName.hpp"

/************************************************************************/
/*
 * This is a class to help serialize things to byte streams:
 *   - create an instance
 *   - call store() to put things into it
 *   - get the result member with the bytes
 *
 * The store() functions return the number of bytes that have
 * been stored.
 *
 * Note: you can also store a protobuf message.
 */

namespace SteamBot
{
    namespace Connection
    {
        class Serializer
        {
        public:
            bool noLogging=false;
            std::vector<std::byte> result;

        public:
            Serializer();
            ~Serializer();
            
        public:
            /* container of bytes, stored as-is */
            template <typename T> size_t store(T bytes) requires std::ranges::input_range<T> && std::same_as<std::byte, std::remove_cv_t<typename T::value_type>>
            {
                auto size=bytes.end()-bytes.begin();
                result.insert(result.end(), bytes.begin(), bytes.end());
                return size;
            }

            /* integer values, stored as little-endian */
            template <typename T> size_t store(T value) requires std::is_integral_v<T>
            {
                const T little=boost::endian::native_to_little(value);
                const std::byte* bytes=static_cast<const std::byte*>(static_cast<const void*>(&little));
                return store(std::span<const std::byte>(bytes, sizeof(little)));
            }

            /* enum values, stored as little-endian as their underlying type */
            template <typename T> size_t store(T value) requires std::is_enum_v<T>
            {
                return store(static_cast<std::underlying_type_t<T>>(value));
            }

            constexpr size_t store()
            {
                return 0;
            }

        public:
            /* varargs template to store multiple values */
            template <typename FIRST, typename... REST> size_t store(FIRST&& first, REST&&... rest)
            {
                size_t result=store(std::forward<FIRST>(first));
                result+=+store(std::forward<REST>(rest)...);
                return result;
            }

        public:
            size_t storeProto(const google::protobuf::MessageLite&);

            template <typename T> size_t storeProto(const T& message) requires (pp::is_message<T>)
            {
                auto bytes=SteamBot::ProtoPuf::encode(message);
                BOOST_LOG_TRIVIAL(debug) << "serializing protopuf message "
                                         << SteamBot::typeName<T>()
                                         << " into " << bytes.size() << " bytes: "
                                         << SteamBot::ProtoPuf::toJson(message);
                return store(bytes);
            }
        };
    }
}

/************************************************************************/
/*
 * A class to deserialize things from byte streams:
 *   - create an instance with the byte stream
 *   - get() things from it
 *   - throws Deserializer::NotEnoughDataException if we run out of data
 *
 * Note: this supports protobuf messages! This can throw a
 * Deserializer::ProtobufException.
 */

namespace SteamBot
{
    namespace Connection
    {
        class Deserializer
        {
        public:
            class NotEnoughDataException : public DataException { };
            class ProtobufException : public DataException { };

        public:
            Base::ConstBytes data;

        public:
            Deserializer(Base::ConstBytes);
            ~Deserializer();

        public:
            /* Get all of the remaining data*/
            Base::ConstBytes getRemaining()
            {
                auto result=data;
                data=decltype(data)();
                return result;
            }

        public:
            /* just extract a stream of bytes */
            Base::ConstBytes getBytes(size_t size)
            {
                if (size>data.size())
                {
                    throw NotEnoughDataException();
                }
                auto result=data.first(size);
                data=data.subspan(size);
                return result;
            }

            /* integer values are read as little-endian */
            template<typename T> T get() requires std::is_integral_v<T>
            {
                const auto little=*static_cast<const T*>(static_cast<const void*>(getBytes(sizeof(T)).data()));
                return boost::endian::little_to_native(little);
            }

            /* enum values are read as little-endian */
            template<typename T> T get() requires std::is_enum_v<T>
            {
                return static_cast<T>(get<std::underlying_type_t<T>>());
            }

        private:
            void get()
            {
            }

        public:
            /* varargs template to get multilpe values */
            template <typename FIRST, typename... REST> void get(FIRST&& first, REST&&... rest) requires std::is_reference_v<FIRST>
            {
                first=get<std::remove_reference_t<FIRST>>();
                get(std::forward<REST>(rest)...);
            }

        public:
            void getProto(google::protobuf::MessageLite&, size_t);

            template <typename T> void getProto(T& message, size_t messageSize) requires (pp::is_message<T>)
            {
                if (messageSize>data.size())
                {
                    throw NotEnoughDataException();
                }

                const std::span<std::byte> bytes(const_cast<std::byte*>(data.data()), messageSize);

                auto result=pp::message_coder<T>::decode(bytes);
                message=std::move(result.first);

                auto size=result.second.data()-bytes.data();
                assert(size<=static_cast<decltype(size)>(messageSize));

                BOOST_LOG_TRIVIAL(debug) << "deserialized protopuf message "
                                         << SteamBot::typeName<T>()
                                         << " from " << size << " bytes: "
                                         << SteamBot::ProtoPuf::toJson(message);

                getBytes(size);
            }
        };
    }
}
