#include "Config.hpp"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string/case_conv.hpp>

/************************************************************************/

static const std::string filename="Config.xml";

/************************************************************************/

static const boost::property_tree::ptree& getConfig()
{
	static const boost::property_tree::ptree config=[](){
		boost::property_tree::ptree tree;
		boost::property_tree::read_xml(filename, tree);
		return tree;
	}();

	return config;
}

/************************************************************************/

static std::string getLowerCase(const std::string& name)
{
	std::string result=getConfig().get<std::string>(name);
	boost::algorithm::to_lower(result);
	return result;
}

/************************************************************************/

SteamBot::Config::SteamAccount::SteamAccount()
	: user(getLowerCase("steam-account.user")),
	  password(getConfig().get<std::string>("steam-account.password"))
{
	BOOST_LOG_TRIVIAL(info) << "using steam account \"" << user << "\"";
}

/************************************************************************/

SteamBot::Config::SteamAccount::~SteamAccount() =default;

/************************************************************************/

const SteamBot::Config::SteamAccount& SteamBot::Config::SteamAccount::get()
{
	static const SteamAccount account;
	return account;
}
