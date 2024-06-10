#include "Client/Client.hpp"
#include "Helpers/StringCompare.hpp"
#include "Helpers/JSON.hpp"
#include "SteamID.hpp"
#include "Vector.hpp"
#include "Settings.hpp"
#include "Modules/Executor.hpp"
#include "Client/ClientInfo.hpp"

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

std::vector<ClientInfo*> ClientInfo::findDisplay(std::string_view name)
{
    std::vector<ClientInfo*> result;
    std::lock_guard<decltype(mutex)> lock(mutex);
    for (auto info : clients)
    {
        if (SteamBot::caseInsensitiveStringCompare_equal(info->displayName(info), name))
        {
            result.push_back(info);
        }
    }
    return result;
}

/************************************************************************/

ClientInfo* ClientInfo::find(std::function<bool(const boost::json::value&)> pred)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    for (auto info : clients)
    {
        auto& dataFile=SteamBot::DataFile::get(info->accountName, SteamBot::DataFile::FileType::Account);
        if (dataFile.examine(pred))
        {
            return info;
        }
    }
    return nullptr;
}

/************************************************************************/

ClientInfo* ClientInfo::find(SteamBot::AccountID accountId)
{
    return find([accountId](const boost::json::value& json) -> bool {
        if (auto item=SteamBot::JSON::getItem(json, "Info", "SteamID"))
        {
            SteamBot::SteamID steamId(item->to_number<SteamBot::SteamID::valueType>());
            if (steamId.getAccountId()==accountId)
            {
                return true;
            }
        }
        return false;
    });
}

/************************************************************************/
/*
 * Searches the ~/.SteamBot directory for data files, and registers
 * the accounts.
 */

void ClientInfo::init()
{
    static const std::regex regex("Account-(([a-z]|[A-Z]|[0-9]|_)+)\\.json");
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
    result.reserve(clients.size());
    for (auto info : clients)
    {
        result.push_back(info);
    }
    return result;
}

/************************************************************************/

std::vector<ClientInfo*> ClientInfo::getGroup(std::string_view name)
{
    auto result=getClients();
    SteamBot::erase(result, [name](const ClientInfo* info) {
        auto& dataFile=SteamBot::DataFile::get(info->accountName, SteamBot::DataFile::FileType::Account);
        bool isMember=dataFile.examine([name](const boost::json::value& json) {
            if (auto array=SteamBot::JSON::getItem(json, "Groups"))
            {
                for (const auto& element : array->as_array())
                {
                    if (element.as_string()==name)
                    {
                        return true;
                    }
                }
            }
            return false;
        });
        return !isMember;
    });
    return result;
}

/************************************************************************/

std::string ClientInfo::prettyName(SteamBot::AccountID accountId)
{
    std::string result;
    if (auto clientInfo=find(accountId))
    {
        result.append(clientInfo->accountName);
        result.append(" (");
        result.append(std::to_string(toInteger(accountId)));
        result.append(")");
    }
    else
    {
        result=std::to_string(toInteger(accountId));
    }
    return result;
}

/************************************************************************/
/*
 * ToDo: this is a bit messed up. It is possible that a new client is
 * launched while we are shutting down the ones we have intiially,
 * resulting in the final wait to never complete.
 */

void ClientInfo::quitAll()
{
    BOOST_LOG_TRIVIAL(info) << "ClientInfo::quitAll";
    for (auto info : getClients())
    {
        if (auto client=info->client)
        {
            SteamBot::Modules::Executor::execute(client, [](SteamBot::Client& client_) {
                client_.quit();
            });
        }
    }
    SteamBot::Client::waitAll();
}

/************************************************************************/

const std::string_view& SteamBot::AccountDisplayName::name() const
{
    static const std::string_view string{"account-display-name"};
    return string;
}

/************************************************************************/

void SteamBot::AccountDisplayName::storeWhiteboard(SteamBot::Settings::Setting::Ptr<> setting) const
{
    SteamBot::Settings::Internal::storeWhiteboard<AccountDisplayName>(std::move(setting));
}

/************************************************************************/

static SteamBot::Settings::Init<SteamBot::AccountDisplayName> accountDisplayNameInit;

/************************************************************************/
/*
 * Again, ClientInfo is a mess.
 *
 * This function exists because I'm locking the mutex for getClient(),
 * which is fine for a standalone displayName() call, but won't work
 * when searching for display names.
 *
 * Hence, this function does NOT lock the mutex; the caller needs
 * to do that for us.
 */

std::string ClientInfo::displayName(const ClientInfo* thisInfo) const
{
    /*
    * First, let's try the whiteboard since that's our official API to
    * settings.
    * We can only use it on the thread that owns it, AND if settings
    * have already been loaded
    */
    {
        auto runningClient=SteamBot::Client::getClientPtr();
        auto thisClient=thisInfo->client.get();
        if (runningClient && thisClient==runningClient && runningClient->isReady())
        {
            if (auto displayName=runningClient->whiteboard.has<SteamBot::AccountDisplayName::Ptr>())
            {
                const auto& result=(*displayName)->value;
                if (!result.empty())
                {
                    return result;
                }
            }
            return accountName;
        }
    }

    // Ok, lets grab the data from the account file
    {
        typedef SteamBot::Settings::Setting Setting;
        std::unique_ptr<Setting> setting;

        SteamBot::Startup::InitBase<Setting>::create([&setting](std::unique_ptr<Setting> created) {
            setting=std::move(created);
        }, [](const SteamBot::Settings::Setting::InitBase& init)
        {
            return &init==&accountDisplayNameInit;
        });

        if (setting)
        {
            SteamBot::DataFile& dataFile=SteamBot::DataFile::get(accountName, SteamBot::DataFile::FileType::Account);
            setting->load(dataFile);

            const auto& item=dynamic_cast<AccountDisplayName&>(*setting);
            if (!item.value.empty())
            {
                return item.value;
            }
        }
    }

    // Still nothing, so it's the accountName
    return accountName;
}

/************************************************************************/
/*
 * For now(?), we only support a setting to change the name, i.e.  no
 * profile names from Steam.
 *
 * This will try to use the whiteboard, if possible.
 *
 * ToDo: this whole display-name thing has turned into more of a mess
 * than I had anticipated...
 */

std::string ClientInfo::displayName() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return displayName(this);
}
