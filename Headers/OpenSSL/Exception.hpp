#pragma once

/************************************************************************/

namespace SteamBot
{
    namespace OpenSSL
    {
        class Exception
        {
        public:
            Exception();
            ~Exception();

        public:
            static void throwMaybe();
            static void throwMaybe(int);

            template <typename T> static T* throwMaybe(T* ptr)
            {
                if (ptr==nullptr)
                {
                    throw Exception();
                }
                return ptr;
            }
        };
    }
}
