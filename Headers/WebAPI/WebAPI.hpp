#pragma once

#include <memory>
#include <string_view>
#include <functional>
#include <boost/url/url.hpp>
#include <boost/url/url_view.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/json.hpp>
#include <list>

/************************************************************************/
/*
 * This models a standard Steam WebAPI request.
 *
 * See https://partner.steamgames.com/doc/webapi_overview for an general
 * overview on these requests.
 *
 * For now, we have name->string and name->array-of-string parameters.
 * The latter has "count" hardcoded as per the website; if that turns
 * out to be bad information I'll add that name as an additional
 * argument.
 *
 * While I haven't seen an actual "responses are in JSON", I'll
 * assume they are... at least for now. Thus, all reponses will
 * be decoded for you.
 */

/************************************************************************/

namespace SteamBot
{
    namespace WebAPI
    {
        class Request
        {
        private:
            boost::urls::url url;

        public:
            Request(std::string_view, std::string_view, unsigned int);
            virtual ~Request();

            boost::json::value send() const;

        public:
            void set(std::string_view, std::string_view);
            void set(std::string_view, const std::vector<std::string>&);

            void set(std::string_view, int);	// for now, let's just assume this is enough
        };
    }
}
