#pragma once

/************************************************************************/
/*
 * Print the current exception to the boost log
 */

namespace SteamBot
{
	namespace Exception
	{
		void log();
	}
}

/************************************************************************/
/*
 * Run a function, log exceptions. Rethrows them.
 */

namespace SteamBot
{
	namespace Exception
	{
		template <typename FUNC> auto log(FUNC function)
		{
			try
			{
				return function();
			}
			catch(...)
			{
				log();
				throw;
			}
		}
	}
}
