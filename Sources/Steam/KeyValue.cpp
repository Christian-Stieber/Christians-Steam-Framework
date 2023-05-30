#include "Steam/KeyValue.hpp"

/************************************************************************/
/*
 * The fileformat for binary serlization seems to be like this:
 *  - a "type byte
 *  - followed by data
 * If the type is "None", it's a parent node. Children will be
 * written out, then an "end" type byte will be written
 *
 * Just like SteamKit2, we only support "String" values for now
 */

/************************************************************************/

enum class DataType : uint8_t
{
	None = 0,
	String = 1,
	// Int32 = 2,
	// Float32 = 3,
	// Pointer = 4,
	// WideString = 5,
	// Color = 6,
	// UInt64 = 7,
	End = 8,
	// Int64 = 10,
};

/************************************************************************/

template <typename T> static std::underlying_type_t<T> toInteger(T value)
{
    return static_cast<std::underlying_type_t<T>>(value);
}

/************************************************************************/

Steam::KeyValue::Node& Steam::KeyValue::Node::getNode(std::string name)
{
	Ptr& child=children[std::move(name)];
	Node* node=dynamic_cast<Node*>(child.get());
	if (child)
	{
		assert(node!=nullptr);
	}
	else
	{
		node=new Node;
		child.reset(node);
	}
	return *node;
}

/************************************************************************/

void serializeStringToBinary(const std::string& string, Steam::KeyValue::BinarySerializationType& bytes)
{
	// we assume it's already UTF-8
	const char* t=string.c_str();
	do
	{
		bytes.push_back(static_cast<std::byte>(*t));
	}
	while (*(t++)!='\0');
}
	
/************************************************************************/

template <> void Steam::KeyValue::serializeToBinary<std::string>(const std::string& name, const std::string& data, Steam::KeyValue::BinarySerializationType& bytes)
{
	bytes.push_back(static_cast<std::byte>(toInteger(DataType::String)));
	serializeStringToBinary(name, bytes);
	serializeStringToBinary(data, bytes);
}

/************************************************************************/

void Steam::KeyValue::Node::serialize(const std::string& name, Steam::KeyValue::BinarySerializationType& bytes) const
{
	bytes.push_back(static_cast<std::byte>(toInteger(DataType::None)));
	serializeStringToBinary(name, bytes);
	for (const auto& child : children)
	{
		child.second->serialize(child.first, bytes);
	}
	bytes.push_back(static_cast<std::byte>(toInteger(DataType::End)));

	// SteamKit writes TWO end markers?
	bytes.push_back(static_cast<std::byte>(toInteger(DataType::End)));
}

/************************************************************************/

Steam::KeyValue::BinarySerializationType Steam::KeyValue::Node::serializeToBinary(const std::string& name) const
{
	BinarySerializationType bytes;
	serialize(name, bytes);
	return bytes;
}
