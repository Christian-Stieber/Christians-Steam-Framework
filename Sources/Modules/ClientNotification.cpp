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
/*
 * Note: for some reason, I couldn't find SteamKit or ASF dealing
 * with these. I couldn't find enums either.
 */

/************************************************************************/

#include "Client/Module.hpp"
#include "Modules/UnifiedMessageServer.hpp"
#include "Modules/UnifiedMessageClient.hpp"
#include "Modules/ClientNotification.hpp"
#include "Helpers/ProtoPuf.hpp"
#include "Helpers/Time.hpp"
#include "EnumString.hpp"

/************************************************************************/

typedef SteamBot::Modules::ClientNotification::Messageboard::ClientNotification ClientNotification;

/************************************************************************/

ClientNotification::ClientNotification() =default;
ClientNotification::~ClientNotification() =default;

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

    private:
        void getNotifications();

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

boost::json::value ClientNotification::toJson() const
{
    boost::json::object json;
    json["id"]=toInteger(notificationId);
    SteamBot::enumToJson(json, "type", type);
    json["timestamp"]=SteamBot::Time::toString(timestamp);
    if (expiry.time_since_epoch().count()!=0) json["expiry"]=SteamBot::Time::toString(expiry);
    json["read"]=read;
    json["body"]=body;
    json["targets"]=targets;
    json["hidden"]=hidden;
    return json;
}

/************************************************************************/

void ClientNotificationModule::getNotifications()
{
    std::shared_ptr<CSteamNotification_GetSteamNotifications_Response> response;
    {
        CSteamNotification_GetSteamNotifications_Request request;
        response=SteamBot::Modules::UnifiedMessageClient::execute<CSteamNotification_GetSteamNotifications_Response>("SteamNotification.GetSteamNotifications#1", std::move(request));
    }

    using namespace pp;
    const auto& notifications=(*response)["notifications"_f];
    for (const auto& item : notifications)
    {
        auto notification=std::make_shared<ClientNotification>();
        notification->notificationId=static_cast<SteamBot::NotificationID>(item["notification_id"_f].value());
        notification->type=static_cast<ClientNotification::Type>(item["notification_type"_f].value());
        notification->timestamp=std::chrono::system_clock::from_time_t(item["timestamp"_f].value());
        notification->expiry=std::chrono::system_clock::from_time_t(item["expiry"_f].value());
        notification->read=item["read"_f].value();
        notification->body=boost::json::parse(item["body_data"_f].value());

        // No real idea what these fields might be
        notification->targets=item["notification_targets"_f].value();
        notification->hidden=item["hidden"_f].value();
        assert(!notification->hidden);		// Let's see if get any hidden ones...

        getClient().messageboard.send(std::move(notification));
    }
}

/************************************************************************/

void ClientNotificationModule::handle(std::shared_ptr<const CSteamNotificationNotificationsReceivedNotificationMessageType> message)
{
    getNotifications();
}

/************************************************************************/

void ClientNotificationModule::run(SteamBot::Client& client)
{
    waitForLogin();

    // We shouldn't need that later, but for now...
    getNotifications();

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
