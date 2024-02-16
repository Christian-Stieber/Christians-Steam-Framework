#pragma once

/************************************************************************/
/*
 * Note: std::regex appears to use a lot of stack on g++
 *
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86164
 */

#undef CHRISTIAN_REGEX
#ifdef __GLIBCXX__
#include <boost/regex.hpp>
#define CHRISTIAN_REGEX boost
#else
#include <regex>
#define CHRISTIAN_REGEX std
#endif
