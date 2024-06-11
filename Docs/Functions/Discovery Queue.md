# Clearing discovery queues

The framework has functionality to deal with discovery queues; note that this means the
user does not get to see them. Some users like to actually use them properly, but there
is no way for to reset the internal state that Steam has on the queue status.

## Clearing one queue

The convenience call
```c++
bool SteamBot::DiscoveryQueue::clear():
```
simply clears one queue, returning `true` for success and `false` for error.

## Clearing the sale queues

Steam usually provides trading cards for clearing sale queues during the major sales;
you can use the call 
```c++
bool SteamBot::SaleQueue::clear():
```
to clear ALL sale queues. Steam occasionally changes how these event-type things work,
and I can't test any of this outside of these events, so this may or may not still
work on the next such sale.

