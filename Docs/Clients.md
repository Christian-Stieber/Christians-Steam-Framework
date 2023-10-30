# Clients

The "Client" is the bot instance for an account. As the framework goes, you can have as many clients as you want, but you should probably exercise caution to avoid annoying Valve.

I might hardcode a limit eventually.

Note: the APIs described below are a bit on the messy side; I want to take a look at them and will probably make some breaking changes eventually. But, this is what you get right now.

Launching a client requires you to provide a `ClientInfo` instance.

## Creating a new ClientInfo

Call `ClientInfo* SteamBot::ClientInfo::create(accountName)` to create a new bot.
This will return a `nullptr` if a bot for that account already exists.

## Getting an existing ClientInfo

Call `ClientInfo * SteamBot::ClientInfo::find(accountName)` to obtain the ClientInfo for an already existing bot.
This will return a `nullptr` if a bot for that account doesn't exist.

## Launching a client

With your `ClientInfo` in hand, call `SteamBot::Client::launch(clientInfo)` to launch the bot.

## Invoking operations on clients

While different APIs will work differently, you'll always have to start with your client: `std::shared_ptr<Client> clientInfo->getClient()`.

Keep in mind that clients run on their own threads; most APIs will at least expect you to be on that already. You can use the `Executor` for this:

```
bool SteamBot::Modules::Executor::execute(std::shared_ptr<SteamBot::Client>, std::function<void(Client&)>);
```

This will block until your code has finished; it returns `true` if your function has completed normally.
See the next section for an example.

You can also wrap your code into a fiber:

```
bool SteamBot::Modules::Executor::executeWithFiber(std::shared_ptr<SteamBot::Client>, std::function<void(Client&)>);
```

This will not block.

## Terminating a client

Invoke `client->quit()`. Example:

```
if (auto client=clientInfo->getClient())
{
    bool success=SteamBot::Modules::Executor::execute(client, [](SteamBot::Client& client) {
        client.quit(false);
    });
}
```

## Deleting a bot

Quit your application (data files will be cached), find the data file in the filesystem, and remove it :-)
Sorry, no framework functions for that right now.
