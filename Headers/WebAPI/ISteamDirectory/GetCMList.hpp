#pragma once

#include "WebAPI/WebAPI.hpp"

#include <chrono>

/************************************************************************/

namespace SteamBot
{
    namespace WebAPI
    {
        namespace ISteamDirectory
        {
            class GetCMList
            {
            public:
                std::chrono::system_clock::time_point when;

				std::vector<std::string> serverlist;

            public:
                GetCMList(unsigned int);		// private, use get() instead
                ~GetCMList();

            public:
                static std::shared_ptr<const GetCMList> get(unsigned int);
            };
        }
    }
}


