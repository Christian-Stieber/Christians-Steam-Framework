#pragma once

/************************************************************************/
/*
 * SteamKit2/Types/KeyValue.cs
 *
 * I tried to use boost::ptree with my own data type, but that
 * didn't work at all... and it doesn't exactly have any kind
 * of documentation.
 *
 * Note that SteamKit MessageObject only uses binary serialization, so
 * I'm only supporting that. Also, their binary serialization only
 * supports a very limited subset of possible KV trees.
 */

#include <vector>
#include <cassert>
#include <string>
#include <memory>
#include <unordered_map>

/************************************************************************/

namespace Steam
{
	namespace KeyValue
	{
		class ItemBase;
		class ValueBase;
		template <typename T> class Value;
		class Node;

		typedef std::vector<std::byte> BinarySerializationType;

		// SteamKit2 can only serialize strings, so I'm putting into a separate template
		template <typename T> void serializeToBinary(const std::string&, const T&, std::vector<std::byte>&)
		{
			assert(false);		// cannot serialize T to binary
		}

		template <> void serializeToBinary<std::string>(const std::string&, const std::string&, std::vector<std::byte>&);

		typedef Node Tree;
	}
}

/************************************************************************/

class Steam::KeyValue::ItemBase
{
public:
	typedef std::unique_ptr<ItemBase> Ptr;

protected:
	ItemBase() =default;

public:
	virtual ~ItemBase() =default;

public:
	virtual void serialize(const std::string&, BinarySerializationType&) const =0;
};

/************************************************************************/

class Steam::KeyValue::ValueBase : public Steam::KeyValue::ItemBase
{
protected:
	ValueBase() =default;

public:
	virtual ~ValueBase() =default;

public:
	virtual void serialize(const std::string&, BinarySerializationType&) const override =0;
};

/************************************************************************/

template <typename T> class Steam::KeyValue::Value : public Steam::KeyValue::ValueBase
{
private:
	T value;

public:
	Value(T value_)
		: value(std::move(value_))
	{
	}

	virtual ~Value() =default;

public:
	virtual void serialize(const std::string& name, BinarySerializationType& bytes) const override
	{
		serializeToBinary<T>(name, value, bytes);
	}
};

/************************************************************************/

class Steam::KeyValue::Node : public Steam::KeyValue::ItemBase
{
private:
	std::unordered_map<std::string, Ptr> children;

public:
	Node() =default;
	virtual ~Node() =default;

public:
	Node& getNode(std::string);

public:
	template <typename T> void setValue(std::string name, T value)
	{
		Ptr& child=children[std::move(name)];
		child.reset(new Value<T>(std::move(value)));
	}

public:
	virtual void serialize(const std::string&, BinarySerializationType&) const override;

	BinarySerializationType serializeToBinary(const std::string&) const;
};
