#pragma once

#include "boost/examples/fiber/asio/yield.hpp"

/************************************************************************/

namespace boost
{
    namespace fibers
    {
        namespace asio
        {
            extern thread_local yield_t yield;
        }
    }
}
