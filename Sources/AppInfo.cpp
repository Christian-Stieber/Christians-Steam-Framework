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

#include "BlockingQuery.hpp"
#include "AppInfo.hpp"
#include "CacheFile.hpp"
#include "Steam/KeyValue.hpp"
#include "Helpers/JSON.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver_appinfo.hpp"

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

    public:
        const boost::json::value* get_noMutex(SteamBot::AppID appId)
        {
            const std::string appIdKey=SteamBot::toString(SteamBot::toInteger(appId));

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
            const std::string appIdKey=SteamBot::toString(SteamBot::toInteger(appId));

            {
                auto iterator=updates.find(appIdKey);
                assert(iterator==updates.end());
            }

            auto success=updates.emplace(std::move(appIdKey), std::move(json)).second;
            assert(success);

            changed=true;
        }

    public:
        void update_noMutex(const std::vector<SteamBot::AppID>&);
        void updateDlcs_noMutex();

    public:
        static AppInfoFile& get()
        {
            static AppInfoFile& file=*new AppInfoFile();
            return file;
        }
    };
}

/************************************************************************/
/*
 * Fetch AppInfo for a list of provided AppIDs.
 * This is mostly just a helper for higher-level update functions that
 * actually determine AppIDs that need updating.
 */

void AppInfoFile::update_noMutex(const std::vector<SteamBot::AppID>& appIds)
{
    if (!appIds.empty())
    {
        auto request=std::make_unique<Steam::CMsgClientPICSProductInfoRequestMessageType>();
        for (const auto appId : appIds)
        {
            auto& app=*(request->content.add_apps());
            app.set_appid(SteamBot::toUnsignedInteger(appId));
        }

        typedef Steam::CMsgClientPICSProductInfoResponseMessageType ResponseType;
        SteamBot::sendAndWait<ResponseType>(std::move(request),[this](std::shared_ptr<const ResponseType> response) -> bool {
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
                            auto json=tree->toJson();
                            BOOST_LOG_TRIVIAL(info) << "obtained appInfo for app-id " << SteamBot::toInteger(appId) << ": " << json;
                            update_noMutex(appId, std::move(json));
                        }
                        else
                        {
                            BOOST_LOG_TRIVIAL(error) << "failed to deserialize KeyValue data";
                        }
                    }
                }
            }
            return !(response->content.has_response_pending() && response->content.response_pending());
        });
    }
}

/************************************************************************/
/*
 * This goes through the exising AppInfo, and fetches DLC AppInfos that
 * are still missing.
 *
 * I don't know whether a DLC can have a DLC as well, so we just continue
 * doing this until no more updates are needed.
 *
 * Note: our entire logic is based around app-ids, not package-ids. Thus,
 * I believe we don't have to check for "dupliates" when generating the
 * list of DLC app-ids, since any given DLC can only be attached to
 * one parent app-id.
 */

void AppInfoFile::updateDlcs_noMutex()
{
    std::vector<SteamBot::AppID> prev;  // check for undetected issues

    while (true)
    {
        std::vector<SteamBot::AppID> dlcs;

        file->examine([this, &dlcs](const boost::json::value& json) -> void {
            const auto& jsonObject=json.as_object();
            for (const auto& item: jsonObject)
            {
                if (auto listofdlcs=SteamBot::JSON::getItem(item.value(), "extended", "listofdlc"))
                {
                    std::string string;
                    const std::string_view view(listofdlcs->as_string());
                    string.reserve(2+view.length());
                    string+='[';
                    string+=view;
                    string+=']';

                    auto array=boost::json::parse(string).as_array();
                    for (const auto& dlc: array)
                    {
                        const auto appId=SteamBot::JSON::toNumber<SteamBot::AppID>(dlc);

                        char key[16];
                        auto result=std::to_chars(key, key+sizeof(key), toInteger(appId));
                        assert(result.ec==std::errc());
                        *(result.ptr)='\0';
                        if (jsonObject.find(key)==jsonObject.end())
                        {
                            dlcs.push_back(appId);
                        }
                    }
                }
            }
        });

        if (dlcs.empty())
        {
            return;
        }

        // endless loop...
        assert(dlcs!=prev);

        {
            std::ostringstream string;
            for (auto appId: dlcs)
            {
                string << ' ' << static_cast<unsigned int>(appId);
            }
            BOOST_LOG_TRIVIAL(info) << "getting AppInfos for DLCs:" << string.view();
        }

        update_noMutex(dlcs);

        prev=std::move(dlcs);
    }
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
    // We need to find SOME way to update existing info that's
    // been updated server-side...

    std::vector<AppID> appIds;
    for (const auto& game : games.games)
    {
        if (appInfoFile.get_noMutex(game.first)==nullptr)
        {
            appIds.push_back(game.first);
        }
    }

    appInfoFile.update_noMutex(appIds);
    appInfoFile.updateDlcs_noMutex();
    appInfoFile.save_noMutex(true);
}

/************************************************************************/

bool SteamBot::AppInfo::examine(std::function<bool(const boost::json::value&)> callback)
{
    auto& appInfoFile=AppInfoFile::get();
    std::lock_guard<decltype(appInfoFile.mutex)> lock(appInfoFile.mutex);

    assert(appInfoFile.file!=nullptr);
    return appInfoFile.file->examine([&callback](const boost::json::value& json) {
        return callback(json);
    });
}

/************************************************************************/

std::optional<boost::json::value> SteamBot::AppInfo::get(std::span<const std::string_view> names)
{
    std::optional<boost::json::value> result;
    examine([&names,&result](const boost::json::value& json) {
        const boost::json::value* item=&json;
        for (const auto& name: names)
        {
            if (auto object=item->if_object())
            {
                if ((item=object->if_contains(name))!=nullptr)
                {
                    continue;
                }
            }
            return false;
        }
        result=*item;
        return true;
    });
    return result;
}
