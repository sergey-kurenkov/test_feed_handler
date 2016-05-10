SYNOPSIS



Write up a console application able to parse and process a file

with market data feed control sequence.





SYNTAX



$ md_replay <file> [<symbol>]



<file>		File with market data control commands

<symbol>        Instrument identification



If <symbol> is omitted, the application must print all data

as dictated by the market data control sequence. Otherwise,

the output must be filtered to show results only for a given

instrument.





CONSTRAINTS



* Program must be written in C++

* It is not allowed to use any frameworks but standard C and

  C++ libraries, STL included

* Program must run under Linux OS

* Source code must be accompanied with a GNU makefile able

  to compile and clean the source in the source directory





FILE FORMAT



File contains a sequence of commands in CSV format, one command

per line, lines are terminated by a Unix end of line separator.



On a line, the first cell is always a command name, followed

by one or more arguments separated by commas.





LIST OF COMMANDS



* ORDER ADD,<order id>,<symbol>,<side>,<quantity>,<price>



  Insert a new order into order book for a given instrument.



* ORDER MODIFY,<order id>,<quantity>,<price>



  Modify price and quantity in an already existing order.



* ORDER CANCEL,<order id>



  Withdraw an existing order.



* SUBSCRIBE BBO,<symbol>



  Starting from this line on, keep printing BBO information for

  a given instrument, unless it is filtered out by a program

  option (see SYNTAX)



* UNSUBSCRIBE BBO,<symbol>



  Starting from this line on, stop printing BBO information for

  a given instrument, unless there are subscribers left or

  unless the symbol is filtered out by a command line option

  (see SYNTAX)



* SUBSCRIBE VWAP,<symbol>,<quantity>



  Starting from this line on, keep printing VWAP information for

  a given instrument, unless it is filtered out by a program

  option (see SYNTAX)



* UNSUBSCRIBE VWAP,<symbol>,<quantity>



  Starting from this line on, stop printing VWAP information for

  a given instrument, unless there are subscribers left or

  unless the symbol is filtered out by a command line option

  (see SYNTAX)



* PRINT,<symbol>



  Print order book price levels for a given symbol in a form of



  Bid                             Ask



  <volume>@<price> | <volume>@<price>



  where <volume> is a cumulative volume for all orders on a

  given price level. Bids must be sorted descending from top

  down, asks should be ascending from top down.



* PRINT_FULL,<symbol>



  Print all orders active in the order book, separated by their

  price levels.





FIELD TYPES



* order id : uint64_t

* symbol   : string

* side     : "Buy" or "Sell"

* quantity : uint64_t

* price    : double





GLOSSARY



* Instrument - a tradable security, for instance, shares of Apple Inc.



* Symbol - Unique instrument identifier, used on a market to mark updates

           for a given security. For Apple Inc. commonly used symbol is

	   "AAPL".



* Order - Request to buy or sell a number of shares in a given security.

          Order usually has an identifier to distinct it from other orders,

	  side (Sell or Buy), price and volume (a number of shares).

	  Example of an order is "Sell 10@200.5 AAPL" which means a request

	  to sell 10 shares of Apple Inc. at a price no lower than $200.5.



* Order book - list of outstanding orders in a given security. Orders are

               kept in two different columns, buys separate from sells, and

	       are sorted as, from top to bottom:



	       - Bids (aka Buys)  : Highest price to lowest price

	       - Asks (aka Sells) : Lowest price to highest price



	       Example of an order book:



		|#orders| volume |   bid   |   ask   | volume |#orders|

		|-----------------------------------------------------|

		|      1|      10|    72.82|    72.85|     100|      1|

		|      1|     100|    72.81|    72.86|     100|      1|

		|      1|     100|     72.8|    72.87|     100|      1|

		|      1|     100|    72.79|    72.88|     100|      1|

		|      1|     100|    72.78|    72.89|     100|      1|

		|      2|    5500|       70|         |        |       |

		|-----------------------------------------------------|



               Note that there are might be multiple orders with the same

               price, in which case they shall all contribute to the same

               order book level (that is, their volumes shall be sum up).



* Best price - Top level of the order book, also referred to as BBO (best

               bid / offer). Includes topmost buy and sell level of the

               order book.



* Bid - Request to buy in a designated security. Also referred to as "Buy"



* Offer - Request to sell in a designated security. Also referred to as

          "Sell", "Ask"



* VWAP - Volume Weighted Average Price, an average price for a given amount

         of shares readily available to trade in the market, represented

	 as a pair of <buy price, sell price>. For instance, for the order

	 book above, VWAP for quantity 5 would have been <72.82, 72.85>,

	 because topmost volume covers all requested quantity. For volume

	 20 it would have been <72.815, 72.85>, because on the ask side

	 there are still enough volume to pick from the topmost level, but

	 for the bid it is



	 bid vwap(20) := (10*72.82 + 10*72.81) / (10 + 10) = 72.815



	 If order book has no volume to cover requested quantity, the

	 side that fails should yield value of NIL.





FURTHER REFERENCES



http://www.investopedia.com



