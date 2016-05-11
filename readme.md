[![Build Status](https://travis-ci.org/skwllsp/test_feed_handler.png)](https://travis-ci.org/skwllsp/test_feed_handler)
[![Coverage Status](https://coveralls.io/repos/skwllsp/test_feed_handler/badge.svg)](https://coveralls.io/r/skwllsp/test_feed_handler)


## SYNOPSIS



Write up a console application able to parse and process a file with market data feed control sequence.

### SYNTAX


``` bash
$ md_replay <file> [<symbol>]

<file> 	    File with market data control commands
<symbol>    Instrument identification

```



If <symbol> is omitted, the application must print all data
as dictated by the market data control sequence. Otherwise,
the output must be filtered to show results only for a given
instrument.


Full task description: https://github.com/skwllsp/test_feed_handler/blob/master/task.txt

