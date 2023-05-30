#include "Client/Client.hpp"
#include "Client/Message.hpp"
#include "EnumString.hpp"
#include "Exception.hpp"

/************************************************************************/

Steam::Client::Dispatcher::Dispatcher(Steam::Client::Client& client_)
	: client(client_)
{
}

/************************************************************************/

Steam::Client::Dispatcher::~Dispatcher() =default;

/************************************************************************/
/*
  * Note: we spawn a new coroutine for each incoming packet
 */

void Steam::Client::Dispatcher::handlePacket(std::span<const std::byte> bytes)
{
	const auto messageType=Steam::Client::Message::Header::Base::peekMessgeType(bytes);
	BOOST_LOG_TRIVIAL(info) << "received message type " << SteamBot::enumToStringAlways(messageType);

	if (messageType==Steam::Client::Message::Type::Multi)
	{
		handleMulti(bytes);
	}
	else
	{
		auto iterator=messages.handlers.find(messageType);
		if (iterator!=messages.handlers.end())
		{
			auto handler=iterator->second.get();
			auto message=handler->deserialize(bytes);

			boost::asio::spawn(client.strand, [this, message=std::move(message), handler=std::move(handler), messageType](boost::asio::yield_context yield) {
				auto quitter=client.quitter.add(yield, [this]() { /* do something? */ });
				BOOST_LOG_TRIVIAL(debug) << "handler for " << SteamBot::enumToStringAlways(messageType) << " message running";
				client.handleException([message=std::move(message), handler=std::move(handler), yield]() {
					handler->call(yield, message);
				});
				BOOST_LOG_TRIVIAL(debug) << "handler for " << SteamBot::enumToStringAlways(messageType) << " message exiting";
			});
		}
		else
		{
			BOOST_LOG_TRIVIAL(info) << "ignoring unexpected message type " << SteamBot::enumToStringAlways(messageType);
		}
	}
}

/************************************************************************/
/*
 * Read messages from the connection and handle them.
 *
 * For now, we don't actually have a termination condition. This will
 * probably have to change at some point.
 */

void Steam::Client::Dispatcher::execute(boost::asio::yield_context yield, Steam::Client::Connection::Base& connection)
{
	static const boost::coroutines::attributes attributes(1024*1024);

	boost::asio::spawn(yield, [this, &connection](boost::asio::yield_context yield) {
		auto quitter=client.quitter.add(yield, [this, &connection]() { connection.cancel(); });
		BOOST_LOG_TRIVIAL(info) << "packet dispatcher running";
		client.handleException([this, &connection, yield]() {
			while (true)
			{
				const auto bytes=connection.readPacket(yield);
				handlePacket(bytes);
			}
		});
		BOOST_LOG_TRIVIAL(info) << "packet dispatcher exiting";
	}, attributes);

	auto quitter=client.quitter.add(yield, [this]() { tasks.cancel(); });
	BOOST_LOG_TRIVIAL(info) << "task dispatcher running";
	while (tasks.execute(yield))
		;
	BOOST_LOG_TRIVIAL(info) << "task dispatcher exiting";
}
