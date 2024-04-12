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

#include "AppInfo.hpp"
#include "CacheFile.hpp"
#include "BlockingQuery.hpp"
#include "Steam/KeyValue.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_appinfo.hpp"

#include <charconv>
#include <cassert>

/************************************************************************/
/*
 * Note: for this one, we just work with the file JSON directly,
 * and only store updates in local data.
 */

namespace
{
    class AppInfoFile : public SteamBot::CacheFile
    {
    private:
        std::unordered_map<std::string /* appId */, boost::json::value> updates;

    private:
        AppInfoFile()
        {
            init("AppInfo");
        }

        virtual ~AppInfoFile() =default;

    private:
        virtual bool updateFile(boost::json::value& fileJson) override
        {
            BOOST_LOG_TRIVIAL(info) << "updating file " << file->name << "\" with " << updates.size() << " new entries";
            for (auto& item: updates)
            {
                fileJson.as_object()[item.first]=std::move(item.second);
            }
            updates.clear();
            return true;
        }

    private:
        static std::string makeString(SteamBot::AppID appId)
        {
            std::string string;
            {
                char appIdString[64];
                auto result=std::to_chars(appIdString, appIdString+sizeof(appIdString)-1, SteamBot::toInteger(appId));
                assert(result.ec== std::errc());
                if (result.ec==std::errc())
                {
                    string.assign(appIdString, result.ptr);
                }
            }
            return string;
        }

    public:
        const boost::json::value* get_noMutex(SteamBot::AppID appId)
        {
            const auto appIdKey=makeString(appId);

            {
                auto iterator=updates.find(appIdKey);
                if (iterator!=updates.end())
                {
                    return &(iterator->second);
                }
            }

            return file->examine([&appIdKey](const boost::json::value& json) -> const boost::json::value* {
                auto iterator=json.as_object().find(appIdKey);
                if (iterator!=json.as_object().end())
                {
                    return &((*iterator).value());
                }
                return nullptr;
            });
        }

    public:
        void update_noMutex(SteamBot::AppID appId, boost::json::value json)
        {
            auto appIdKey=makeString(appId);

            {
                auto iterator=updates.find(appIdKey);
                assert(iterator==updates.end());
            }

            auto success=updates.emplace(std::move(appIdKey), std::move(json)).second;
            assert(success);

            changed=true;
        }

    public:
        static AppInfoFile& get()
        {
            AppInfoFile& file=*new AppInfoFile();
            return file;
        }
    };
}

/************************************************************************/

void SteamBot::AppInfo::update(const SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames& games)
{
    auto& appInfoFile=AppInfoFile::get();
    std::lock_guard<decltype(appInfoFile.mutex)> lock(appInfoFile.mutex);

    // For now, only request appInfo that we don't already have
    //
    // ToDo: can we somehow request only updated info?
    // Or should we just request everything all the time?
    std::unique_ptr<Steam::CMsgClientPICSProductInfoRequestMessageType> request;
    {
        for (const auto& game : games.games)
        {
            if (appInfoFile.get_noMutex(game.first)==nullptr)
            {
                if (!request)
                {
                    request.reset(new Steam::CMsgClientPICSProductInfoRequestMessageType());
                }
                auto& app=*(request->content.add_apps());
                app.set_appid(SteamBot::toUnsignedInteger(game.first));
            }
        }
    }

    if (request)
    {
        if (auto response=SteamBot::sendAndWait<Steam::CMsgClientPICSProductInfoResponseMessageType>(std::move(request)))
        {
            for (int i=0; i<response->content.apps_size(); i++)
            {
                auto& app=response->content.apps(i);
                if (app.has_appid())
                {
                    auto appId=static_cast<SteamBot::AppID>(app.appid());

                    if (app.has_buffer())
                    {
                        std::string_view buffer(app.buffer());
                        if (!buffer.empty() && buffer.back()=='\0')
                        {
                            buffer.remove_suffix(1);
                        }

                        std::string name;
                        if (auto tree=Steam::KeyValue::deserialize(buffer, name))
                        {
                            assert(name=="appinfo");
                            BOOST_LOG_TRIVIAL(info) << "obtained appInfo for app-id " << SteamBot::toInteger(appId);
                            appInfoFile.update_noMutex(appId, tree->toJson());
                        }
                        else
                        {
                            BOOST_LOG_TRIVIAL(error) << "failed to deserialize KeyValue data";
                        }
                    }
                }
            }
        }
    }

    appInfoFile.save_noMutex(true);
}
