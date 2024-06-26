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
#include "DataFile.hpp"
#include "MiscIDs.hpp"

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
        DataFile& dataFile;

	public:
        static void launch(ClientInfo&);
        static void waitAll();

        static void waitForLogin();

	public:
		static std::shared_ptr<Client> getClientShared();
		static Client* getClientPtr();
		static Client& getClient();

		void launchFiber(std::string, std::function<void()>);
        void quit(bool restart=false);

        bool isQuitting() const
        {
            return quitMode!=QuitMode::None;
        }

    public:
        std::string getDisplayName() const;

    private:
        mutable boost::fibers::mutex statusMutex;
        mutable boost::fibers::condition_variable statusCondition;

        enum class Status : uint8_t { Initializing, Ready };
        Status status=Status::Initializing;

        void waitReady() const;

    public:
        bool isReady() const
        {
            assert(getClientPtr()==this);
            return status!=Status::Initializing;
        }

	private:
        mutable boost::fibers::mutex modulesMutex;
        std::unordered_map<std::type_index, std::shared_ptr<Module>> modules;

        ClientInfo& clientInfo;

    private:
        enum class QuitMode : uint8_t { None, Restart, Quit };
        QuitMode quitMode=QuitMode::None;

	private:
        void initModules();

	public:
		Client(ClientInfo&);		// internal use!!!
		~Client();

    public:
        ClientInfo& getClientInfo() const
        {
            return clientInfo;
        }

        template <typename T> std::shared_ptr<T> getModule() const
        {
            waitReady();
            std::lock_guard<decltype(modulesMutex)> lock(modulesMutex);
            auto iterator=modules.find(std::type_index(typeid(T)));
            assert(iterator!=modules.end());
            return std::dynamic_pointer_cast<T>(iterator->second);
        }
	};
}
