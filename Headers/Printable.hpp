#pragma once

#include <boost/json.hpp>

#include <iostream>

/************************************************************************/
/*
 * This is an attempt to help make standardized printable objects.
 *
 * The JSON produced here is meant for output, so it need not be able
 * to allow for a roundtrip conversion.
 */

namespace SteamBot
{
    class Printable
    {
    protected:
        Printable() =default;

    public:
        virtual ~Printable() =default;
        virtual boost::json::value toJson() const =0;
    };

    inline std::ostream& operator<<(std::ostream& stream, const Printable& printable)
    {
        return stream << printable.toJson();
    }
}
