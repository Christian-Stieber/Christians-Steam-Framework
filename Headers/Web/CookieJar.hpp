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

#include <string_view>
#include <memory>
#include <vector>

#include <boost/beast/http/fields.hpp>
#include <boost/url/url_view.hpp>
#include <boost/json/value.hpp>
#include <boost/fiber/mutex.hpp>

/************************************************************************/

namespace SteamBot
{
    namespace Web
    {
        class CookieJar
        {
        public:
            class Cookie
            {
            public:
                std::string domain;
                bool includeSubdomains=false;

                std::string path;

            public:
                std::string name;
                std::string value;

            private:
                void setDomain(const std::vector<std::string_view>&, const boost::urls::url_view_base*);
                void setContent(std::string_view);

            public:
                // Takes a set-cookie header.
                // May throw InvalidCookieException.
                Cookie(std::string_view, const boost::urls::url_view_base* url=nullptr);

                // Takes a name and value
                Cookie(std::string_view, std::string_view, const boost::urls::url_view_base* url=nullptr);

                ~Cookie();

            public:
                boost::json::value toJson() const;
                bool match(const Cookie&) const;
            };

        private:
            mutable boost::fibers::mutex mutex;

            std::vector<std::unique_ptr<const Cookie>> cookies;

        private:
            bool store(std::unique_ptr<Cookie>&&);

        public:
            CookieJar();
            ~CookieJar();

        public:
            boost::json::value toJson() const;

        public:
            // The URL that we have accessed
            // The headers from the reuslt
            // Returns whether anything was updated
            bool update(const boost::urls::url_view&, const boost::beast::http::fields&);

            bool update(std::unique_ptr<Cookie>);

        public:
            // Iterate over matching cookies
            void iterate(const boost::urls::url_view_base&, std::function<void(const Cookie&)>) const;

            // Return the string for the cookie header
            std::string get(const boost::urls::url_view_base&) const;

            // Return the value for a cookie, if it exists
            std::string get(const boost::urls::url_view_base&, std::string_view) const;

        public:
            // Access the cookies for the calling thread.
            // We don't sotre this in the whiteboard, since whiteboard items are const.
            static std::shared_ptr<CookieJar> get();
        };
    }
}
