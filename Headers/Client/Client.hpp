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

#include <boost/asio/io_context.hpp>

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
    class Client
	{
    public:
        class Module;

    public:
        /*
         * Put this into the whiteboard before quitting the client, to
         * restart the client.
         *
         * Note: I tried to restart it on the same thread, but the
         * fiber/asio stuff seems to be a bit flaky. So, at least for
         * now, you're getting a new thread.
         */
        class RestartClient {};

    public:
        Cancel cancel;
        Whiteboard whiteboard;
        Messageboard messageboard;
        const Universe& universe;
        DataFile dataFile;

	public:
		static void launch();
		static void waitAll();

	public:
		static Client& getClient();

        boost::asio::io_context& getIoContext() { return *ioContext; }

		void launchFiber(std::string, std::function<void()>);
        void quit();

	private:
		std::shared_ptr<boost::asio::io_context> ioContext;
        std::unordered_map<std::type_index, std::unique_ptr<Module>> modules;
		unsigned int fiberCount=0;
        bool quitting=false;

	private:
		void main();
        void initModules();
		Client();

	public:
		~Client();

    public:
        template <typename T> T& getModule() const
        {
            auto iterator=modules.find(std::type_index(typeid(T)));
            assert(iterator!=modules.end());
            return dynamic_cast<T&>(*(iterator->second));
        }
	};
}
