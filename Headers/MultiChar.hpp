#pragma once

/************************************************************************/

namespace Steam
{
	namespace Client
	{
		consteval uint32_t makeMultiChar(char a, char b, char c, char d)
		{
			return ((static_cast<unsigned char>(a)<<24)+
					(static_cast<unsigned char>(b)<<16)+
					(static_cast<unsigned char>(c)<<8)+
					(static_cast<unsigned char>(d)<<0));
		}
	}
}
