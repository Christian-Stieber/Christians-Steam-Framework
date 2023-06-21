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

/************************************************************************/

Steam::KeyValue::Node* Steam::KeyValue::Node::getNode(const std::string& name) const
{
    Node* result=nullptr;
    auto iterator=children.find(name);
    if (iterator!=children.end())
    {
        result=dynamic_cast<Node*>(iterator->second.get());
        assert(result!=nullptr);
    }
    return result;
}

/************************************************************************/

Steam::KeyValue::Node& Steam::KeyValue::Node::createNode(std::string name)
{
    Node* result;
    auto& child=children[std::move(name)];
    if (child)
    {
        result=dynamic_cast<Node*>(child.get());
        assert(result!=nullptr);
    }
    else
    {
        result=new Node();
        child.reset(result);
    }
    return *result;
}

/************************************************************************/

boost::json::value Steam::KeyValue::Node::toJson() const
{
    boost::json::object json;
    for (const auto& child : children)
    {
        json[child.first]=child.second->toJson();
    }
    return json;
}
