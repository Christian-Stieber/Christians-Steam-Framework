#include "Client/Client.hpp"
#include "Helpers/StringCompare.hpp"

#include <filesystem>
#include <regex>

/************************************************************************/
/*
 * Note: ClientInfo can't currently be destructed, because
 * its lifetime is not managed.
 *
 * Maybe, some later time.
 */

/************************************************************************/

typedef SteamBot::ClientInfo ClientInfo;

/************************************************************************/

static std::mutex mutex;
static std::vector<ClientInfo*> clients;

/************************************************************************/

ClientInfo::ClientInfo(std::string accountName_)
    : accountName(accountName_)
{
}

/************************************************************************/

ClientInfo::~ClientInfo()
{
    // For now(?), we can't delete ClientInfo
    assert(false);
}

/************************************************************************/

bool ClientInfo::isActive() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return active;
}

/************************************************************************/

bool ClientInfo::setActive(bool state)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    bool current=active;
    active=state;
    return current;
}

/************************************************************************/

std::shared_ptr<SteamBot::Client> ClientInfo::getClient() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return client;
}

/************************************************************************/

void ClientInfo::setClient(std::shared_ptr<SteamBot::Client> client_)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    client=std::move(client_);
}

/************************************************************************/
/*
 * Does not lock the mutex
 */

static ClientInfo* doFind(std::string_view accountName)
{
    for (auto info : clients)
    {
        if (SteamBot::caseInsensitiveStringCompare_equal(info->accountName, accountName))
        {
            return info;
        }
    }
    return nullptr;
}

/************************************************************************/
/*
 * Returns nullptr if the account name already exists
 */

ClientInfo* ClientInfo::create(std::string accountName)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (doFind(accountName))
    {
        return nullptr;
    }
    auto info=new ClientInfo(std::move(accountName));
    clients.push_back(info);
    return info;
}

/************************************************************************/

ClientInfo* ClientInfo::find(std::string_view accountName)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return doFind(accountName);
}

/************************************************************************/
/*
 * Searches the ~/.SteamBot directory for data files, and registers
 * the accounts.
 */

void ClientInfo::init()
{
    std::regex regex("Account-(([a-z]|[A-Z]|[0-9]|_)+)\\.json");
    for (auto const& entry: std::filesystem::directory_iterator{"."})
    {
        if (entry.is_regular_file())
        {
            auto filename=entry.path().filename().string();
            std::smatch matchResults;
            if (std::regex_match(filename, matchResults, regex))
            {
                assert(matchResults.size()==3);
                std::string accountName=matchResults[1].str();
                auto info=create(std::move(accountName));
                assert(info!=nullptr);
            }
        }
    }
}

/************************************************************************/

std::vector<ClientInfo*> ClientInfo::getClients()
{
    std::vector<ClientInfo*> result;
    std::lock_guard<decltype(mutex)> lock(mutex);
    for (auto info : clients)
    {
        result.push_back(info);
    }
    return result;
}
