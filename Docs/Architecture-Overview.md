# Central threads

The framework runs two main threads at this stage:

* the `Asio`-thread hosts all network-I/O -- that is, the webclient and the Steam client connections
* the `UI`-thread hosts the UI

# Client threads

Each account/client is run in a separate thread as well.

Many functional blocks of the clients are wrapped into "modules" which are collected at program launch and started as clients are created. These modules will generally launch a fiber.

## Fibers

For the most part, your modules will launch fibers through `Client::LaunchFiber`. This has two advantages over calling `boost::fibers::fiber()` yourself:
* fibers will catch and discard `OperationCancelled` exceptions
* fibers will be counted, causing the client to wait for them to quit during shutdown

## Communication inside a client thread

While I'm not entirely happy about this, so I might redo it eventually, the "event loop" inside a client is based on a `Waiter`.

Basically, a waiter is a single object that has a `wait()` call, which is usually run inside an endless loop (hence the fibers).

A waiter doesn't do anything on its own. You can create waiter-items that are attached to the waiter. The `wait()` call will return when at least one of the attached items has awoken.

You'll have to check each wait item individually to learn what has happened (hence my headaches about the system -- there are no callbacks). Don't forget to check any -- if an item has fired, it will remain awoken until its reset, which usually happens by querying its information.
If that doesn't happen, the fiber will go into an endless loop because the `wait()` immediately returns, and because fibers are cooperative, the client will be stuck.

### The `Whiteboard`

The whiteboard is a data container for a client. Items are identified by type (so, don't store `std::string` or something like that), and you can wait for items to change.

Note that inheritance does not work here: queried types must be an exact match.

### The `Messageboard`

This is similar to the whiteboard, except that it doesn't store data: when a message is sent, the waiter items for that message type will fire and their fibers will be able to retrieve the message from them.

Similar to the whiteboard, inheritance doesn't work: you can't register a base class and get all messages derived from it.

### The `HTTPClient`

While http queries are actually run on the `Asio`-thread, you don't need to worry about this. If you use the official API to make a query, it will be posted to the `Asio`-thread for you, and the result will be communicated through a waiter items.

### The UI

When you request input from the UI (for a password or a SteamGuard code) you will, again, get a waiter item to retrieve the result.

## Quitting

Cancellation objects can be used to help with fiber shutdown. When you create a cancellation object, you attach it to a 'cancelable' object; also, make sure the lifetime of your cancellation object reflects the lifetime of your thing.

Attaching the cancellation object to a waiter will make the waiter throw an `OperationCancelled` exception; usually, you don't have to do anything as `Client::launchFiber` will catch and ignore that for you.

When the client wants to quit, it cancels all cancellation objects, then waits for the cancellation objects to disappear, then waits for all known fibers to disappear.

