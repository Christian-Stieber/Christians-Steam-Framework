/*
 * This file is part of "Christians-Steam-Framework"
 * Copyright (C) 2023- Christian Stieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.  If not see
 * <http://www.gnu.org/licenses/>.
 */

#include "../Console.hpp"

/************************************************************************/

namespace
{
    class Buffer
    {
    private:
        std::string_view string;

    public:
        Buffer(std::string_view string_)
            : string(string_)
        {
        }

    public:
        char peek() const
        {
            if (string.size()==0)
            {
                return '\0';
            }
            return string.front();
        }

    public:
        char get()
        {
            const auto c=peek();
            if (c!='\0')
            {
                string.remove_prefix(1);
            }
            return c;
        }
    };
}

/************************************************************************/

namespace
{
    class WordList
    {
    private:
        std::vector<std::string> words;
        std::string word;

    public:
        void end(bool allowEmpty=false)
        {
            if (allowEmpty || word.size()>0)
            {
                words.emplace_back(std::move(word));
                word.clear();
            }
        }

        void append(char c)
        {
            word.push_back(c);
        }

        bool empty() const
        {
            return word.empty();
        }

    public:
        operator decltype(words)()
        {
            return std::move(words);
        }
    };
}

/************************************************************************/

static bool isWhitespace(char c)
{
    return c==' ' || c=='\t' || c=='\n' || c=='\0';
}

/************************************************************************/
/*
 * Split the line into words.
 *
 * Words are usually whitespace-separated, BUT:
 *  - we support "\" as an escape character that will unconditionally
 *    consider the next character to be part of the word
 *  - we support quoted strings
 */

std::vector<std::string> SteamBot::UI::CLI::getWords(std::string_view line)
{
    WordList word;
    Buffer buffer(line);

    bool escaped=false;
    char quoted='\0';

    while (true)
    {
        char c=buffer.get();
        if (c=='\0')
        {
            word.end(false);
            return word;
        }
        else
        {
            if (escaped)
            {
                word.append(c);
            }
            else
            {
                if (!quoted && isWhitespace(c))
                {
                    word.end(false);
                }
                else if (c==quoted && isWhitespace(buffer.peek()))
                {
                    word.end(true);
                }
                else if (c=='\\')
                {
                    escaped=true;
                }
                else if (!quoted && (c=='"' || c=='\'') && word.empty())
                {
                    quoted=c;
                }
                else
                {
                    word.append(c);
                }
            }
        }
    }
}
