#include "OpenSSL/Exception.hpp"

#include <openssl/err.h>
#include <openssl/bio.h>

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::OpenSSL::Exception Exception;

/************************************************************************/

static void logErrors()
{
	static const bool initialized=[](){
        ERR_load_crypto_strings();
        return true;
    }();

    assert(ERR_peek_error()!=0);

	BIO* bio=BIO_new(BIO_s_mem());
	ERR_print_errors(bio);
    {
        const auto written=BIO_write(bio, "", 1);
        assert(written==1);
    }
    char* data;
    const auto size=BIO_get_mem_data(bio, &data);

    BOOST_LOG_TRIVIAL(error) << "OpenSSL error: " << std::string_view(data, size);

    BIO_free(bio);
}

/************************************************************************/

Exception::Exception()
{
    logErrors();
}

/************************************************************************/

Exception::~Exception() =default;

/************************************************************************/

void Exception::throwMaybe()
{
	if (ERR_peek_error()!=0)
	{
		throw Exception();
	}
}

/************************************************************************/

void Exception::throwMaybe(int result)
{
	if (result<=0)
	{
		throw Exception();
	}
}
