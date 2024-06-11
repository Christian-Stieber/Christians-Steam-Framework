# Client modules

The framework uses "modules" to attach long-running functionality to clients. Each module is wrapped into a fiber; when a client starts, all modules are instanciated and run. When a client exits, it waits for all module-fibers to shut down.

A module essentially  looks like this:

```c++
#include "Client/Module.hpp"

class MyModule : public SteamBot::Client::Module
{
public:
   MyModule() =default;
   virtual ~MyModule() =default;

   virtual void init(SteamBot::Client&) override;
   virtual void run(SteamBot::Client&) override;
};

MyModule::Init<MyModule> myInit;
```

I'm usually wrapping this into an anonymous namespace, since it doesn't need to visible.

Modules are handled like this by the system:

* On application startup, all modules found in the code are collected
* When a client starts:
  1. all modules are instanciated
  2. `init` is called on all modules
  3. a fiber is created for each module and invokes the `run` function

Note that modules are handled in unspecified order.

For your convenience, each module has a `waiter` instance that you can use for your event loop:

```c++
typedef SteamBot::Modules::OwnedGames::Messageboard::GameChanged GameChanged;

class MyModule : public SteamBot::Client::Module
{
private:
   SteamBot::Messageboard::WaiterType<GameChanged> gameChangedWaiter;
   ...

public:
   void handle(std::shared_ptr<const GameChanged>);
};

void MyModule::init(SteamBot::Client& client)
{
   gameChangedWaiter=client.messageboard.createWaiter<GameChanged>(*waiter);
}

void MyModule::run(SteamBot::Client& client)
{
   while(true)
   {
      waiter->wait();
      gameChangedWaiter->handle(this);
   }
}
```

Things to look out for:
* Make sure that your module responds to client quits. This is generally handled automatically, since blocking APIs provided by the framework register cancellation objects and throw `OperationCancelled` exceptions which are caught and ignored by the fiber code; therefore, don't use things like plain `boost::fibers::condition_variable` to wait for an extended period of time.
* You are running on a fiber, which run cooperatively -- which means you MUST wait/yield eventually. In particular, if you forget to handle a waiter wakeup, the `wait()` will immediately return because there is an active event, causing an endless loop that will completely block the client.
* Also, avoid blocking the thread -- only block your calling fiber. There are exceptions to this, like we don't have fiber-support for file-I/O, but keep in mind that thread-level blocks will block the entire client, not just your module that's making the call.
