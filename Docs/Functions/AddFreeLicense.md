# Adding free licenses

This lets you add free licenses to an account.

## `SteamBot::Modules::AddFreeLicense`

This only has two convenience functions:
```c++
void add(AppID);
void add(PackageID);
```
These are using very different APIs on Steam. The one using the `PackageID` is generally safer, because
it doesn't rely on Steam having to "guess" which package to add; on the downside, users usually have to
do a little more research to find the `PackageID`.

Both will run asynchronously, and neither has a success/failure indicator. In fact, I don't even get
such information from the Steam APIs (or haven't been able to figure out how to get it). Users can
watch the output from this and other modules for indicators that a new game as been added to the
account; in particular, when adding a `PackageID`, this module will monitor license updates and
report a matching license regardless of whether it was added or already existed.
