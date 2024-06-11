# Communicating with Steam

After a login, there a three communication modules available to access various Steam features.

## ProtoBuf messages

You can send/receive protobuf-backed messages over the client connection.

Note that for some of these APIs, Steam also has a "web flavored" variant using API keys; while I don't specifically support or recommend that (I won't use API keys while I can avoid it), their documentation can often be used for the protobuf variant as well.

You can find the protofiles in `steamdatabase/protofiles/steam`.

For the most part, you'll want to create typedefs similar to the ones found in [these headers](/Headers/Steam/ProtoBuf) to combine a header, payload, and message code into a C++ type.

## Handling request/response in a blocking fashion

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
class MyModule : public SteamBot::Client::Module
{
private:
   SteamBot::Messageboard::WaiterType<Steam::CMsgClientLoggedOffMessageType> logoffMessageWaiter;

   void init(SteamBot::Client& client) override
   {
      logoffMessageWaiter=client.messageboard.createWaiter<Steam::CMsgClientLoggedOffMessageType>(*waiter);
   }
};
```

Note that by doing this, you'll also register the message code with the protobuf class, which allows the logging to show readable message dumps. Unregistered messages will just be logged with their message code.

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

Note that gooe protobuf has a "service" feature, which is also used by Steam to decribe their RPC. However, I have been unable to make use of this.

To use a (client-)service, start by bridging the protobuf service into the framework:

```c++
#include "steamdatabase/protobufs/steam/steammessages_cloud.steamclient.pb.h"

void example()
{
   typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Cloud::EnumerateUserFiles)> EnumerateUserFilesInfo;
```
This will access the rpc-call `EnumerateUserFiles` from the `Cloud` service (defined in `steammessages_cloud.steamclient.proto`).

This `Info` type provides two typedefs, for the request and response message:

```c++
   std::shared_ptr<EnumerateUserFilesInfo::ResultType> response;
   {
        EnumerateUserFilesInfo::RequestType request;
        request.set_appid(760);
        response=SteamBot::Modules::UnifiedMessageClient::execute<EnumerateUserFilesInfo::ResultType>("Cloud.EnumerateUserFiles#1", std::move(request));
   }
```

The response will simply be a protobuf message for you to process:

```c++
   std::vector<File> files;
   files.reseerve(response->total_files());
   ...
```
