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

#include "Web/CookieJar.hpp"
#include "Web/Cookies.hpp"
#include "Helpers/StringCompare.hpp"
#include "Client/Client.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * https://developer.mozilla.org/en-US/docs/Web/HTTP/Cookies
 */

/************************************************************************/

typedef SteamBot::Web::CookieJar CookieJar;
typedef CookieJar::Cookie Cookie;

/************************************************************************/

namespace
{
    class InvalidCookieException { };
}

/************************************************************************/

CookieJar::CookieJar() =default;
CookieJar::~CookieJar() =default;

Cookie::~Cookie() =default;

/************************************************************************/
/*
 * Check whether domain/path/name match.
 */

bool Cookie::match(const Cookie& other) const
{
    return
        name==other.name &&
        path==other.path &&
        SteamBot::caseInsensitiveStringCompare(domain, other.domain)==std::weak_ordering::equivalent;
}

/************************************************************************/

static void trimSpaces(std::string_view& item)
{
    while (item.size()>0 && item.front()==' ')
    {
        item.remove_prefix(1);
    }

    while (item.size()>0 && item.back()==' ')
    {
        item.remove_suffix(1);
    }
}

/************************************************************************/
/*
 * Split the string into semicolon-separated chunks
 */

static std::vector<std::string_view> split(std::string_view string)
{
    std::vector<std::string_view> result;

    std::string_view item(string.data(),0);
    while (true)
    {
        char c;
        if (string.size()>0)
        {
            c=string.front();
            string.remove_prefix(1);
        }
        else
        {
            c='\0';
        }

        if (c==';' || c=='\0')
        {
            trimSpaces(item);
            result.push_back(item);
            item=std::string_view(string.data(), 0);
            if (c=='\0')
            {
                return result;
            }
        }
        else
        {
            item=std::string_view(item.data(), item.size()+1);
            assert(item.back()==c);
        }
    }
}

/************************************************************************/
/*
 * "String" is the first item of the set-cookie, i.e. the name=value
 */

void Cookie::setContent(std::string_view string)
{
    auto equals=string.find('=');
    if (equals!=std::string_view::npos && equals>0)
    {
        {
            std::string_view view(string.data(), equals);
            trimSpaces(view);
            name=view;
        }

        {
            std::string_view view(string.data()+equals+1, string.size()-equals-1);
            trimSpaces(view);
            value=view;
        }
    }
    else
    {
        value=string;
    }
}

/************************************************************************/

static std::string_view findItem(const std::vector<std::string_view>& items, std::string_view name)
{
    std::string_view result;
    for (const auto& item: items)
    {
        auto equals=item.find('=');
        if (equals!=std::string_view::npos && equals>0)
        {
            std::string_view view(item.data(), equals);
            trimSpaces(view);
            if (SteamBot::caseInsensitiveStringCompare_equal(name, view))
            {
                result=std::string_view(item.data()+equals+1, item.size()-equals-1);
                trimSpaces(result);
                break;
            }
        }
    }
    return result;
}

/************************************************************************/

void Cookie::setDomain(const std::vector<std::string_view>& items, const boost::urls::url_view_base* url)
{
    domain=findItem(items, "Domain");
    if (domain.empty())
    {
        if (url==nullptr || url->host_type()!=boost::urls::host_type::name)
        {
            throw InvalidCookieException();		// missing domain
        }
        domain=url->host_name();
    }
    if (domain.front()!='.')
    {
        domain.insert(0,".");
    }
}

/************************************************************************/
/*
 * https://stackoverflow.com/questions/1969232/what-are-allowed-characters-in-cookies
 */

Cookie::Cookie(std::string_view string, const boost::urls::url_view_base* url)
{
    auto items=split(string);

    if (items.size()==0)
    {
        throw InvalidCookieException();
    }

    // first item is the cookie itself
    setContent(items[0]);
    items.erase(items.begin());

    setDomain(items, url);

    // ToDo: handle the other stuff...
}

/************************************************************************/

bool CookieJar::update(std::unique_ptr<CookieJar::Cookie> cookie)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return store(std::move(cookie));
}

/************************************************************************/

static bool checkDomain(std::string_view host, std::string_view domain)
{
    if (!domain.empty())
    {
        assert(domain[0]=='.');

        if (domain.length()-1==host.length())
        {
            domain.remove_prefix(1);
        }

        if (domain.length()<=host.length())
        {
            std::string_view end=host;
            end.remove_prefix(host.length()-domain.length());
            if (SteamBot::caseInsensitiveStringCompare_equal(end, domain))
            {
                return true;
            }
        }
    }
    return false;
}

/************************************************************************/

void CookieJar::iterate(const boost::urls::url_view_base& url, std::function<void(const Cookie&)> callback) const
{
    if (url.host_type()==boost::urls::host_type::name)
    {
        const auto host=url.host_name();
        std::lock_guard<decltype(mutex)> lock(mutex);
        for (const auto& cookie : cookies)
        {
            if (checkDomain(host, cookie->domain))
            {
                // ToDo: check path

                callback(*cookie);
            }
        }
    }
}

/************************************************************************/
/*
 * Note: you need to lock the mutex yourself
 *
 * Returne true if something was changed.
 */

bool CookieJar::store(std::unique_ptr<Cookie>&& cookie)
{
    bool wasUpdated=false;
    bool found=false;

    for (auto cookieIterator=cookies.begin(); cookieIterator!=cookies.end(); ++cookieIterator)
    {
        if (cookieIterator->get()->match(*cookie))
        {
            found=true;
            if (cookie->value.empty())
            {
                cookies.erase(cookieIterator);
                wasUpdated=true;
            }
            else
            {
                if (cookieIterator->get()->value!=cookie->value)
                {
                    cookieIterator->operator=(std::move(cookie));
                    wasUpdated=true;
                }
            }
            break;
        }
    }
    if (!found)
    {
        cookies.push_back(std::move(cookie));
        wasUpdated=true;
    }
    return wasUpdated;
}

/************************************************************************/

bool CookieJar::update(const boost::urls::url_view& url, const boost::beast::http::fields& headers)
{
    bool wasUpdated=false;
    auto setCookies=headers.equal_range(boost::beast::http::field::set_cookie);
    std::lock_guard<decltype(mutex)> lock(mutex);
    for (auto setIterator=setCookies.first; setIterator!=setCookies.second; ++setIterator)
    {
        try
        {
            if (store(std::make_unique<Cookie>(setIterator->value(), &url)))
            {
                wasUpdated=true;
            }
        }
        catch(const InvalidCookieException&)
        {
            // Let's just ignore them
            BOOST_LOG_TRIVIAL(error) << "Invalid set-cookie: " << setIterator->value();
        }
    }
    return wasUpdated;
}

/************************************************************************/

boost::json::value Cookie::toJson() const
{
    boost::json::object json;
    json["name"]=name;
    json["value"]=value;
    if (!domain.empty()) json["domain"]=domain;

#if 0
    // not supported yet
    json["includeSubdomains"]=includeSubdomains;
    json["path"]=path;
#endif

    return json;
}

/************************************************************************/

boost::json::value CookieJar::toJson() const
{
    boost::json::array json;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        for (const auto& cookie : cookies)
        {
            json.push_back(cookie->toJson());
        }
    }
    return json;
}

/************************************************************************/

std::shared_ptr<CookieJar> CookieJar::get()
{
    SteamBot::Client::getClient();	// asserts that we are on a client thread
    static thread_local std::shared_ptr<CookieJar> cookies=std::make_shared<CookieJar>();
    return cookies;
}

/************************************************************************/

std::string CookieJar::get(const boost::urls::url_view_base& url) const
{
    std::string result;
    iterate(url, [&result](const Cookie& cookie){
        setCookie(result, cookie.name, cookie.value);
    });
    return result;
}

/************************************************************************/

std::string CookieJar::get(const boost::urls::url_view_base& url, std::string_view name) const
{
    std::string result;
    iterate(url, [&result, &name](const Cookie& cookie){
        if (cookie.name==name)
        {
            result=cookie.value;
        }
    });
    return result;
}
