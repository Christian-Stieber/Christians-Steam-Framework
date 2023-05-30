#include "WebAPI/WebAPI.hpp"
#include "HTTPClient.hpp"

#include <charconv>
#include <boost/json/stream_parser.hpp>
#include <boost/log/trivial.hpp>

/************************************************************************/

static const boost::urls::url_view defaultBaseAddress("https://api.steampowered.com");

/************************************************************************/

static std::string makeString(int value)
{
    std::array<char, 64> buffer;
    const auto result=std::to_chars(buffer.data(), buffer.data()+buffer.size(), value);
    assert(result.ec==std::errc());
    return std::string(buffer.data(), result.ptr);
}

/************************************************************************/

SteamBot::WebAPI::Request::Request(std::string_view interface, std::string_view method, unsigned int version)
    : url(defaultBaseAddress)
{
    url.segments().push_back(interface);
    url.segments().push_back(method);
    {
        static const std::string versionPrefix("v");
        url.segments().push_back(versionPrefix+makeString(static_cast<int>(version)));
    }
}

/************************************************************************/

SteamBot::WebAPI::Request::~Request() =default;

/************************************************************************/

void SteamBot::WebAPI::Request::set(std::string_view key, std::string_view value)
{
    assert(!url.params().contains(key));
    url.params().set(key, value);
}

/************************************************************************/

void SteamBot::WebAPI::Request::set(std::string_view key, int value)
{
    set(std::move(key), makeString(value));
}

/************************************************************************/

void SteamBot::WebAPI::Request::set(std::string_view key, const std::vector<std::string>& values)
{
    set("count", static_cast<int>(values.size()));
    for (int i=0; i<values.size(); i++)
    {
        std::string itemKey{key};
        itemKey+='[';
        itemKey+=makeString(i);
        itemKey+=']';
        set(itemKey, values[i]);
    }
}

/************************************************************************/
/*
 * Sends the query.
 * Returns the json-decoded response, or throws.
 */

boost::json::value SteamBot::WebAPI::Request::send() const
{
    const auto response=SteamBot::HTTPClient::query(url.buffer()).get();

    boost::json::stream_parser parser;
    const auto buffers=response->response.body().cdata();
    for (auto iterator=boost::asio::buffer_sequence_begin(buffers); iterator!=boost::asio::buffer_sequence_end(buffers); ++iterator)
    {
        parser.write(static_cast<const char*>((*iterator).data()), (*iterator).size());
    }
    parser.finish();
    boost::json::value result=parser.release();
    BOOST_LOG_TRIVIAL(debug) << "WebAPI request " << url.buffer() << " has returned " << result;
    return result;
}
