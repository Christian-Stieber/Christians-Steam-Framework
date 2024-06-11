# Communicating with Steam

After a login, there a three communication modules available to access various Steam features.

## ProtoBuf messages

You can send/receive protobuf-backed messages over the client connection.

Note that for some of these APIs, Steam also has a "web flavored" variant using API keys; while I don't specifically support or recommend that (I won't use API keys while I can avoid it), their documentation can often be used for the protobuf variant as well.

You can find the protofiles [here](/steamdatabase/protofiles/steam).

For the most part, you'll want to create typedefs similar to the ones found in [these headers](/Headers/Steam/ProtoBufs) to combine a header, payload, and message code into a C++ type.

### Sending protobuf messages

As you can imagine, sending a message means allocating one, filling its payload, and sending it off.

Example:
```c++
   auto message=std::make_unique<Steam::CMsgClientHelloMessageType>();
   message->content.set_protocol_version(12345);
   SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
```

### Receiving protobuf messages

To receive messages, you need to subscribe to them using a `Waiter`; if you are in a module you
can just use the module-provided waiter for this:

```c++
class MyModule
{
private:
   SteamBot::Messageboard::WaiterType<Steam::CMsgClientLoggedOffMessageType> logoffMessageWaiter;

   void init(SteamBot::Client& client) override
   {
      logoffMessageWaiter=client.messageboard.createWaiter<Steam::CMsgClientLoggedOffMessageType>(*waiter);
   }
};
```

Note that by doing this, you'll also register the message code with the C++ type and the protobuf class, which allows the logging to show readable message dumps.

With this in place, incoming messages of that type will be posted on the messageboard, so you can use it however you want.

```c++
class MyModule
{
public:
   void handle(std::shared_ptr<const Steam::CMsgClientLoggedOffMessageType> message)
   {
      int32_t result=message->content.eresult();
   }

private:
   virtual void run(SteamBot::Client&)
   {
      while (true)
      {
         waiter->wait();
         logoffMessageWaiter->handle(this);
      }
   }
```

## Web-API

Sometimes, you'll need to access webpages as your user; the framework provides an API to add necessary session cookies into your https request.

## Unified Messages (client)

Steam provides a system of "remote procedure calls" to provide a request/response flow. At this stage, the framework still views this as request and response messages; it dos not try to provide a function call interface. However, the API is blocking, for some basic function-call feeling.
