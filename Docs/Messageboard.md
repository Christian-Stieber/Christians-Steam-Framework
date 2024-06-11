# The client messageboard

Each client has a "messageboard" to publish messages. Messages are not stored; anyone not subscribed to
a message type will never receive it, even if they subscribe later.

## Thread-safety

The messageboard is NOT thread-safe. There is NO way to use it across threads in any way.

## Identifying messages

Messages are identified by type.

## Sending messages

Messages are sent like this:
```c++
auto message=std::make_shared<YourType>(...);
...
getClient().messageboard.send(std::move(message));
```
Well, I guess the takeaway is that you provide an `std::shared_ptr` with your message.

## Receiving messages

Note that inheritance doesn't work here -- you cannot subscribe to a base class and receive
messages from derived classes. Subscriptions must be precise type matches.

To receive messages, you must subscribe to their type, so this will often happen
inside a module.

```c++
typedef SteamBot::Modules::OwnedGames::Messageboard::GameChanged GameChanged;

class MyModule : public SteamBot::Client::Module
{
private:
   SteamBot::Messageboard::WaiterType<GameChanged> gameChangedWaiter;

public:
   void handle(std::shared_ptr<const GameChanged> message)
   {
   }

private:
   virtual void init(SteamBot::Client& client) override
   {
      gameChangedWaiter=client.messageboard.createWaiter<GameChanged>(*waiter);
   }

   virtual void run(SteamBot::Client& client) override
   {
      while(true)
      {
         waiter->wait();
         gameChangedWaiter->handle(this);
      }
   }
}:
```

The usual way to process incoming messages is by providing a `handle()` function on
your module. A common mistake is to declare these as `private` -- unfortunately, they
need to be `public` for the waiter item to actually call it.
