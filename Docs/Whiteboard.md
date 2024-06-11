# The client whiteboard

Each client has a "whiteboard" to store (non-persistent) data. This basically acts as a collection of global variables, except that
* no references are created to the module providing the data (so this does not necessarily pull in the module)
* you can subscribe to an item, so changes are posted to your event loop

## Thread-safety

The whiteboard is NOT thread-safe. There is NO way to use it across threads in any way.

## Identifying items

Whiteboard items are identified by type; there are no names. Thus, don't store plain `std::string`, `int`,
`bool` etc. into it -- always wrap it into a class. On the upside, since types are unique within a program,
we also get unique "names" automatically.

## Storing items

To store an item, simply call something like `getClient().whiteboard.set<YourClass>(yourItem)`. To ensure that the
correct type is stored, provide it as an explicit parameter.

Do NOT change items in the whiteboard unless you know what you're doing. The whiteboard cannot
detect such changes and will not notify subscribers

A useful technique for storing large items is to store a `std::shared_ptr<const YourType>`; this will
make it easy for users to "copy" the item if they need to preserve it across a wait (which is often
the case when processing large datasets)

## Retrieving items

While there a several calls to access whiteboard data, they generally return references to items rather than copies.
However, since the whiteboard has no locking mechanisms, the data can change anytime -- but, since the whiteboard
is not thread-safe and clients run on cooperative fibers, the data is guaranteed to not change until you yield/wait.
Make a copy yourself if you need it longer.

* `const YourType* data=whiteboard.has<YourType>();`\
  this returns a pointer to the item, or `nullptr` if none is stored
* `const YourType& data=whiteboard.get<YourType>(const YourType& default);`\
  this returns a reference to the item, or the default item if none is stored
* `const YourType& data=whiteboard.get<YourType>();`\
  this returns a reference to the item, or `assert(false);` if none is stored

## Subscribing to items

The whiteboard can provide events when an item is changed. Here is an example for typical usage in a client module:

```c++
typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;

class MyModule : public public SteamBot::Client::Module
{
private:
   SteamBot::Whiteboard::WaiterType<OwnedGamers::Ptr> ownedGamesWaiter;

   virtual void init(SteamBot::Client& client) override
   {
      ownedGamesWaiter=client.whiteboard.createWaiter<OwnedGames::Ptr>(*waiter);
   }

   virtual void run(SteamBot::Client& client) override
   {
      while(true)
      {
         waiter->wait();
         if (ownedGamesWaiter->isWoken())
         {
            // item has changed
            // here we just make sure to reset the awoken status
            ownedGamesWaiter->has();
         }
      }
   }
};
```
Note that the waiter item has calls that mimic the retrieval calls from the whiteboard
itself, except that the type is already known from the waiter. Using the calls
on the waiter item will reset the "awoken" status. This will still give you the
current data, NOT the data that initially triggered the wakeup.

Do not blindly use the waiter item retrieval calls to access data; be mindful of the
"awoken" status if you expect notifications about changes. Any access through the
waiter item will reset the awoken status and thus prevent you from seeing it on the
event loop.
