#include "Logging.hpp"

#include <boost/log/core.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

/************************************************************************/

void SteamBot::Logging::init()
{
    boost::log::add_file_log
    (
        boost::log::keywords::file_name = "SteamBot_%N.log",
        boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
        boost::log::keywords::format = "[%TimeStamp%]: %Message%",
        boost::log::keywords::auto_flush = true
    );

    boost::log::add_common_attributes();
}
