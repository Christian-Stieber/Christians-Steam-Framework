# Licenses

This is a bit chaotic and somewhat in flux. As such, it might be appropiate to look at it again
eventually, and maybe restructure it a bit.

In general, Steam uses licenses/packages (identified by a `SteamBot::PackageID`) to provide one or
more games, and Steam actually sends a list of licenses (not games) when purchases are made, which
we use an indicator to update both license and game data.

## `SteamBot::Modules::LicenseList::Whiteboard::Licenses`

This is a table providing some basic information on a `PackageID`. Currently, it provides
* the payment method used to obtain this license
* the purchase date/time

More information is provided by Steam, but its currently ignored because I didn't need it.

While this table is available in the whiteboard, there's also a convenience call to return the
`LicenseInfo` for a given `PackageID`.

## `SteamBot::Modules::LicenseList::Messageboard::NewLicenses`

This message is sent after the licenses have been updated, to provide a list of new `PackageID`s.

## `SteamBot::Modules::PackageData::Whiteboard::PackageData`

The framework obtains additional package information for licenses; when such an update
is completed, the PackageData item will be updated. This provides no data, except for
a pointer to the license list; this allows to determine whether the current package data
is current or not.

To actually access package information, a convenience function is available that takes
an `AppID`, and returns a list of `PackageInfo` items

This contains some information on a given license/package; most important is the list
of `AppID`s provided by this package and the `PackageID` that it describes; this means
it links the packages and games. There's also some more data that I'm not currently
interested in.

## `SteamBot::Modules::PackageInfo::Info`

This is yet another class to provide some information on packages -- specifically, the
name. This item is surprisingly "secret" when it comes to Steam, which makes it difficult
and slow to retrieve so I'm using a separate module for this.

There is a convenience call taking a `PackageID` and returning an `Info` if we have one.
Since it only provides a name for the package, not getting this item should not be considered
a failure.

Note that package names will be cached in a disk file, since retrieving this data is slow.
However, there are still a few packages in my main account that I can't get a name for, so
some packages might never return a name.
