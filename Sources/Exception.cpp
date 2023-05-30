#include "Exception.hpp"

#include <exception>
#include <boost/log/trivial.hpp>
#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

void SteamBot::Exception::log()
{
	BOOST_LOG_TRIVIAL(error) << "exception: " << boost::current_exception_diagnostic_information();
}
