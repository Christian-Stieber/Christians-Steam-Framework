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

#include "Connection/Base.hpp"

/************************************************************************/

namespace SteamBot
{
    namespace OpenSSL
    {
        class AESCryptoBase;
    }
}

/************************************************************************/
/*
 * Encrypted connections use another connection to actually
 * send/receive data.
 *
 * In SteamKit2, these are in EnvelopeEncryptedConnection.cs
 */

namespace SteamBot
{
    namespace Connection
    {
        class Encrypted : public Base
        {
        private:
            /* the transport-connection */
            const std::unique_ptr<Base> connection;

            /* encryption state */
            std::unique_ptr<SteamBot::OpenSSL::AESCryptoBase> encryptionEngine;
            enum class EncryptionState { None, Challenged, Encrypting } encryptionState=EncryptionState::None;

            std::vector<std::byte> readBuffer;	// since readPacket() only returns an std::span... maybe not the best idea

        private:
            void handleEncryptRequest(ConstBytes);
            void handleEncryptResult(ConstBytes);
            void establishEncryption();

        public:
            Encrypted(std::unique_ptr<Base>);
            virtual ~Encrypted();

        public:
            virtual void connect(const Endpoint&) override;
            virtual void disconnect() override;
            virtual MutableBytes readPacket() override;
            virtual void writePacket(ConstBytes) override;

            virtual void getLocalAddress(Endpoint&) const override;

            virtual void cancel() override;
        };
    }
}
