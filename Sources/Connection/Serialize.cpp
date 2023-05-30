#include "Connection/Serialize.hpp"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/util/json_util.h>

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Connection::Serializer Serializer;
typedef SteamBot::Connection::Deserializer Deserializer;

/************************************************************************/

Serializer::Serializer() =default;
Serializer::~Serializer() =default;

/************************************************************************/

Deserializer::Deserializer(SteamBot::Connection::Base::ConstBytes myData)
    : data(myData)
{
}

Deserializer::~Deserializer() =default;

/************************************************************************/
/*
 * Extracts information (name and content as json) from a protobuf
 * message. Used for debugging outputs.
 */

namespace
{
    class ProtobufDebug
    {
    public:
        std::string name;
        std::string json;

        ProtobufDebug(const google::protobuf::MessageLite& protobufMessage)
        {
            auto message=dynamic_cast<const google::protobuf::Message*>(&protobufMessage);
            if (message!=nullptr)
            {
                static const google::protobuf::util::JsonPrintOptions options=[](){
                    google::protobuf::util::JsonPrintOptions options;
                    // options.always_print_primitive_fields=true;
                    return options;
                }();

                google::protobuf::util::MessageToJsonString(*message, &json, options);
                name=message->GetTypeName();
            }
        }
    };
}

/************************************************************************/

size_t Serializer::storeProto(const google::protobuf::MessageLite& protobufMessage)
{
	const auto messageSize=protobufMessage.ByteSizeLong();
	const auto currentIndex=result.size();
	result.resize(currentIndex+messageSize);
	if (!protobufMessage.SerializeToArray(result.data()+currentIndex, messageSize))
	{
		throw std::runtime_error("error while serializing protobuf messsage");
	}

	{
		const ProtobufDebug info(protobufMessage);
		BOOST_LOG_TRIVIAL(debug) << "serializing protobuf message " << info.name << " into " << messageSize << " bytes: " << info.json;
	}

	return messageSize;
}

/************************************************************************/

void Deserializer::getProto(google::protobuf::MessageLite& protobufMessage, size_t messageSize)
{
	if (messageSize>data.size())
	{
		throw NotEnoughDataException();
	}

	google::protobuf::io::ArrayInputStream stream(data.data(), messageSize);
	if (!protobufMessage.ParseFromZeroCopyStream(&stream))
	{
		throw ProtobufException();
	}

	{
		const ProtobufDebug info(protobufMessage);
		BOOST_LOG_TRIVIAL(debug) << "deserialized protobuf message " << info.name << " from " << messageSize << " bytes: " << info.json;
	}

	const auto bytesRead=stream.ByteCount();
	assert(bytesRead<=data.size());
	getBytes(bytesRead);
}
