#pragma once

#include <vector>

/************************************************************************/
/*
 * Erase items from a vector by moving the last item into the
 * erased positions. This means the ordering changes.
 */

/************************************************************************/

namespace SteamBot
{
    template <typename T, typename PRED> size_t erase(std::vector<T>& vector, PRED pred)
    {
        size_t count=0;
        size_t i=0;
        while (i<vector.size())
        {
            if (pred(vector[i]))
            {
                if (i+1<vector.size())
                {
                    vector[i]=std::move(vector.back());
                }
                vector.pop_back();
                count++;
            }
            else
            {
                i++;
            }
        }
        return count;
    }
}
