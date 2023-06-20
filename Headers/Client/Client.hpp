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

#include "Client/Cancel.hpp"
#include "Client/Whiteboard.hpp"
#include "Client/Messageboard.hpp"
#include "Client/DataFile.hpp"
#include "Client/Counter.hpp"

/************************************************************************/

namespace SteamBot
{
    class Universe;
}

/************************************************************************/
/*
 * This is the main client "container"
 */

namespace SteamBot
{
    class ClientInfo;

    class Client
	{
    public:
        class Module;
        class Module;

    public:
        Cancel cancel;
        Whiteboard whiteboard;
        Messageboard messageboard;
        const Universe& universe;
        DataFile dataFile;

	public:
        static void launch(ClientInfo&);
        static void waitAll();

	public:
		static Client* getClientPtr();
		static Client& getClient();

		void launchFiber(std::string, std::function<void()>);
        void quit(bool restart=false);

	private:
        mutable boost::fibers::mutex modulesMutex;
        std::unordered_map<std::type_index, std::shared_ptr<Module>> modules;

        Counter fiberCounter;
        ClientInfo& clientInfo;

    private:
        enum class QuitMode : uint8_t { None, Restart, Quit };
        QuitMode quitMode=QuitMode::None;

	private:
        void main();
        void initModules();

	public:
		Client(ClientInfo&);		// internal use!!!
		~Client();

    public:
        const ClientInfo& getClientInfo() const
        {
            return clientInfo;
        }

        template <typename T> std::shared_ptr<T> getModule() const
        {
            std::lock_guard<decltype(modulesMutex)> lock(modulesMutex);
            auto iterator=modules.find(std::type_index(typeid(T)));
            assert(iterator!=modules.end());
            return std::dynamic_pointer_cast<T>(iterator->second);
        }
	};
}

/************************************************************************/

namespace SteamBot
{
    class ClientInfo
    {
    private:
        std::shared_ptr<Client> client;

    public:
        const std::string accountName;

    public:
        ClientInfo(std::string);		// internal use
        ~ClientInfo();

    public:
        void setClient(std::shared_ptr<Client>);	// internal use

    public:
        std::shared_ptr<Client> getClient() const;

        static void init();

        static ClientInfo* create(std::string);
        static ClientInfo* find(std::string_view);

    public:
        static std::vector<ClientInfo*> getClients();
    };
}
