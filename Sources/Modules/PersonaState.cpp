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

#include "Modules/Connection.hpp"
#include "Modules/PersonaState.hpp"
#include "Client/Module.hpp"
#include "Steam/PersonaState.hpp"
#include "EnumFlags.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_friends.hpp"

/************************************************************************/
/*
 * For now, this is hardcoded
 */

static const auto state=SteamBot::PersonaState::Online;
static const auto stateFlags=SteamBot::EPersonaStateFlag::None;

/************************************************************************/

namespace
{
    class PersonaStateModule : public SteamBot::Client::Module
    {
    private:
        void setState();

    public:
        PersonaStateModule() =default;
        virtual ~PersonaStateModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    PersonaStateModule::Init<PersonaStateModule> init;
}

/************************************************************************/

void PersonaStateModule::setState()
{
    auto message=std::make_unique<Steam::CMsgClientChangeStatusMessageType>();
    message->content.set_persona_state(static_cast<uint32_t>(state));
    message->content.set_persona_state_flags(static_cast<uint32_t>(stateFlags));

    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
}

/************************************************************************/

void PersonaStateModule::run(SteamBot::Client&)
{
    waitForLogin();
    setState();
}

/************************************************************************/

void SteamBot::Modules::PersonaState::use()
{
}
