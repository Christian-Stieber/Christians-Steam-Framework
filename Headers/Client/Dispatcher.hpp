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

#pragma once

#include <memory>
#include <functional>
#include <span>
#include <cstddef>
#include <unordered_map>
#include <utility>
#include <typeindex>
#include <list>

#include <boost/log/trivial.hpp>

#include "Client/Connection.hpp"
#include "Client/Message.hpp"
#include "AsyncTaskQueue.hpp"

/************************************************************************/
/*
 * This is the dispatcher for incoming Steam messages.
 * Client-modules can register for message types.
 */

/************************************************************************/

namespace Steam
{
	namespace Client
	{
		class Dispatcher;
		class Client;
	}
}

/************************************************************************/
/*
 * The "Dispatcher" runs an eventloop on a connection to read
 * incoming packets and send them to handlers.
 *
 * A message handler is a function with the signature
 *   void handler(boost::asio::yield_context, std::shared_ptr<MessageType>);
 * MessageType is a Steam::Client::Message::Message, so it's
 * a full message with header.
 */

class Steam::Client::Dispatcher
{
public:
	class HandlerBase
	{
	protected:
		HandlerBase() =default;

	public:
		virtual ~HandlerBase() =default;

	public:
		virtual std::shared_ptr<Steam::Client::Message::Base> deserialize(const std::span<const std::byte>&) const =0;
		virtual void call(boost::asio::yield_context, std::shared_ptr<Steam::Client::Message::Base>) const =0;
	};

	template <typename T> class Handler : public HandlerBase
	{
	public:
		typedef std::function<void(boost::asio::yield_context, std::shared_ptr<T>)> HandlerType;

	private:
		HandlerType handler;

	public:
		Handler(HandlerType&& myHandler)
			: handler(std::move(myHandler))
		{
		}

		virtual ~Handler() =default;

	private:
		virtual std::shared_ptr<Steam::Client::Message::Base> deserialize(const std::span<const std::byte>& bytes) const override
		{
			return std::make_shared<T>(bytes);
		}

		virtual void call(boost::asio::yield_context context, std::shared_ptr<Steam::Client::Message::Base> baseMessage) const override
		{
			auto message=std::dynamic_pointer_cast<T>(baseMessage);
			handler(context, std::move(message));
		}
	};

private:
	Steam::Client::Client& client;

	struct
	{
		std::unordered_map<Message::Type, std::unique_ptr<HandlerBase>> handlers;
	} messages;

	SteamBot::AsyncTaskQueue tasks;

private:
	void handlePacket(std::span<const std::byte>);
	void handleMulti(std::span<const std::byte>);

public:
	Dispatcher(Steam::Client::Client&);
	~Dispatcher();

public:
	// Very customizable handler. You'll generally want to use one of
	// the other setHandler() variants.
	void setHandler(Steam::Client::Message::Type messageType, std::unique_ptr<HandlerBase> handler)
	{
		const bool success=messages.handlers.try_emplace(messageType, std::move(handler)).second;
		assert(success);
	}

public:
	// Register a handler. Each message-type can only have one handler.
	template <typename T> requires std::is_base_of_v<Steam::Client::Message::Base,T> void setHandler(Handler<T>::HandlerType handler)
	{
		setHandler(T::messageType, std::make_unique<Handler<T>>(std::move(handler)));
	}

public:
	// convenience wrapper to call a member function on object
	template <typename T, typename U> requires std::is_base_of_v<Steam::Client::Message::Base,T>
	void setHandler(U* object, void(U::*function)(boost::asio::yield_context, std::shared_ptr<T>))
	{
		setHandler<T>(std::bind_front(function, object));
	}

	// convenience wrapper to call a member function 'handleMessage' on object
	template <typename T, typename U> requires std::is_base_of_v<Steam::Client::Message::Base,T>
	void setHandler(U* object)
	{
		void(U::*function)(boost::asio::yield_context, std::shared_ptr<T>)=&U::handleMessage;
		setHandler<T>(std::bind_front(function, object));
	}

	// sets a "no-op" handler. This is not particularly useful, but it does show the decoded message in the log
	template <typename T> requires std::is_base_of_v<Steam::Client::Message::Base,T>
	void setNoopHandler()
	{
		setHandler<T>([](boost::asio::yield_context, std::shared_ptr<T>){});
	}

public:
	template <typename FUNC> void queue(FUNC function)
	{
		tasks.push(std::move(function));
	}

public:
	void execute(boost::asio::yield_context, Connection::Base&);
};
