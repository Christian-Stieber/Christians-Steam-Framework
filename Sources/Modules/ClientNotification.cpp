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

/************************************************************************/

#include "Client/Module.hpp"
#include "Modules/UnifiedMessageServer.hpp"
#include "Modules/ClientNotification.hpp"
#include "Helpers/ProtoPuf.hpp"
#include "UI/UI.hpp"

/************************************************************************/
/*
 * Again, the problem with webui protobufs...
 */

namespace
{
    using CSteamNotification_GetSteamNotifications_Request = pp::message<
        pp::bool_field<"include_hidden", 1>,
        pp::int32_field<"language", 2>,
        pp::bool_field<"include_confirmation_count", 3>,
        pp::bool_field<"include_pinned_counts", 4>,
        pp::bool_field<"include_read", 5>,
        pp::bool_field<"count_only ", 6>
        >;

    using SteamNotificationData = pp::message<
        pp::uint64_field<"notification_id", 1>,
        pp::uint32_field<"notification_targets", 2>,
        pp::int32_field<"notification_type", 3>,
        pp::string_field<"body_data", 4>,
        pp::bool_field<"read", 7>,
        pp::uint32_field<"timestamp", 8>,
        pp::bool_field<"hidden", 9>,
        pp::uint32_field<"expiry", 10>
        >;

    using CSteamNotification_GetSteamNotifications_Response = pp::message<
        pp::message_field<"notifications", 1, SteamNotificationData, pp::repeated>,
        pp::int32_field<"confirmation_count", 2>,
        pp::uint32_field<"pending_gift_count", 3>,
        pp::uint32_field<"pending_friend_count", 5>,
        pp::uint32_field<"unread_count", 6>
        >;

#if 0
    using CSteamNotification_NotificationsReceived_Notification = pp::message<
        >;
#else
    using CSteamNotification_NotificationsReceived_Notification = void;
#endif
}

/************************************************************************/

typedef SteamBot::Modules::UnifiedMessageServer::Internal::NotificationMessage<CSteamNotification_NotificationsReceived_Notification> CSteamNotificationNotificationsReceivedNotificationMessageType;

/************************************************************************/

namespace
{
    class ClientNotificationModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<CSteamNotificationNotificationsReceivedNotificationMessageType> notificationReceived;

    public:
        void handle(std::shared_ptr<const CSteamNotificationNotificationsReceivedNotificationMessageType>);

    public:
        ClientNotificationModule() =default;
        virtual ~ClientNotificationModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    ClientNotificationModule::Init<ClientNotificationModule> init;
}

/************************************************************************/

void ClientNotificationModule::init(SteamBot::Client& client)
{
    notificationReceived=client.messageboard.createWaiter<CSteamNotificationNotificationsReceivedNotificationMessageType>(*waiter);
    SteamBot::Modules::UnifiedMessageServer::registerNotification<CSteamNotificationNotificationsReceivedNotificationMessageType>("SteamNotificationClient.NotificationsReceived#1");
}

/************************************************************************/

void ClientNotificationModule::handle(std::shared_ptr<const CSteamNotificationNotificationsReceivedNotificationMessageType> message)
{
    SteamBot::UI::OutputText output;
    output << "received a SteamNotificationClient.NotificationsReceived#1 notification";
}

/************************************************************************/

void ClientNotificationModule::run(SteamBot::Client& client)
{
    while (true)
    {
        waiter->wait();
        notificationReceived->handle(this);
    }
}

/************************************************************************/

void SteamBot::Modules::ClientNotification::use()
{
}
