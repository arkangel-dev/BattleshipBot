# uwe-battleship-bots
This the battleships bot that was provided by UWE for the BSc (Hons) Computer Science program. This is also the same one given by Villa College.

## Important notes
These are some important notes on the game mechanics of the server.

- The speed of ships are locked at 2.22 units / tick. (unit as in a single unit out of the 1000 x 1000 playing grid and tick as in every time the server refreshes and asks the client for input)
- The firing range of a ship is 100 and the visible range of a ship is 200
- In the documentation, it says the type of ships have a rock-paper-scissors affect on each other. But what actually happens is that submarines cannot see frigates and frigates cannot see battle ships and battle ships cannot see submarines.
- In the `send_message()` function, you can only send message to a single user per tick.
