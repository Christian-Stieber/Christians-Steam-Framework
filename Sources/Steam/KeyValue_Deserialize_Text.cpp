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

#include "Steam/KeyValue.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/

namespace
{
    class Tokenizer
    {
    public:
        class SyntaxError { };

    public:
        enum class Token { End, OpenBracket, CloseBracket, String };

    private:
        std::string_view text;

    public:
        Tokenizer(const std::string_view& text_)
            : text(text_)
        {
        }

    private:
        void skipWhitespace()
        {
            while (!text.empty())
            {
                char c=text.front();
                if (c!=' ' && c!='\t' && c!='\n' && c!='\r')
                {
                    return;
                }
                text.remove_prefix(1);
            }
        }

    private:
        char getChar()
        {
            if (text.empty())
            {
                throw SyntaxError();
            }

            char c=text.front();
            text.remove_prefix(1);
            return c;
        }

    public:
        Token operator()(std::string& value)
        {
            skipWhitespace();
            if (text.empty()) return Token::End;

            char c=getChar();

            if (c=='{')
            {
                return Token::OpenBracket;
            }

            if (c=='}')
            {
                return Token::CloseBracket;
            }

            if (c=='"')
            {
                value.clear();
                while ((c=getChar())!='"')
                {
                    if (c=='\\') c=getChar();
                    value+=c;
                }
                return Token::String;
            }

            throw SyntaxError();
        }
    };
}

/************************************************************************/

namespace
{
    class Deserializer
    {
    private:
        Tokenizer tokenizer;
        unsigned int depth=0;

    private:
        Tokenizer::Token token;
        std::string value;
        std::string name;

    public:
        Deserializer(const std::string_view& text)
            : tokenizer(text)
        {
        }

    public:
        void parse(Steam::KeyValue::Node& parent)
        {
            while ((token=tokenizer(value))!=Tokenizer::Token::End)
            {
                switch(token)
                {
                case Tokenizer::Token::String:
                    {
                        name=std::move(value);
                        token=tokenizer(value);
                        switch(token)
                        {
                        case Tokenizer::Token::String:
                            parent.setValue(std::move(name), std::move(value));
                            break;

                        case Tokenizer::Token::OpenBracket:
                            {
                                depth++;
                                auto& child=parent.createNode(std::move(name));
                                parse(child);
                                depth--;
                            }
                            break;

                        default:
                            throw Tokenizer::SyntaxError();
                        }
                    }
                    break;

                case Tokenizer::Token::CloseBracket:
                    if (depth>0) return;
                    throw Tokenizer::SyntaxError();

                case Tokenizer::Token::End:
                    return;

                default:
                    throw Tokenizer::SyntaxError();
                }
            }
        }
    };
}

/************************************************************************/

std::unique_ptr<Steam::KeyValue::Node> Steam::KeyValue::deserialize(std::string_view text, std::string& name)
{
    std::unique_ptr<Node> result;
    try
    {
        Node fake;
        Deserializer(text).parse(fake);

        assert(fake.children.size()==1);
        auto root=fake.children.begin();
        {
            auto node=dynamic_cast<Node*>(root->second.get());
            assert(node!=nullptr);
            root->second.release();
            result.reset(node);
        }
        name=root->first;
    }
    catch(const Tokenizer::SyntaxError&)
    {
        BOOST_LOG_TRIVIAL(debug) << "invalid KeyValue data";
    }
    return result;
}
