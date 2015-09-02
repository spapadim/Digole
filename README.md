Digole display library
======================

This is a quick rewrite of Digole's [original `DigoleSerial`](http://www.digole.com/images/file/Tech_Data/DigoleSerial.zip) library.

***Caveat emptor:***
This library has only been tested by me in [my own projects](https://github.com/spapadim/ESPClock).

--------

While useful, the original was a bit(?) too messy, had some logical flaws (e.g., overriding non-virtual methods from `Print`), and touchscreen support was somewhat incomplete (probably feature was a late-addition).

In addition, I needed gratuitous excuses for a few things.  First, play a bit more deeply with the protocols, SPI, I2C, etc.
Second, to play with the idea of compile-time inheritance I had recently seen in a [very cool Hackaday project](http://hackaday.io/project/6038).
I ended up "accidentally" buying a logic analyzer and learning more about the linking process, which was fun.
<!-- (little did I know I'd spend a week and end up buying yet another cool toy, a logic analyzer :). --->
<!--- (it turned out I knew less about the linking process than I tought I did, and I ended up adding `define`s on top, but.. it was fun :) --->

In particular, turns out I knew less about the linking process than I tought I did (talk about false assumptions all these years!); maybe more in a blog post.
U8glib/ucglib uses a neat trick, placing each font constant in a sub-section of it's own. Perhaps I could play similar tricks, but that'd be taking gratuintousness *way* too far! :)  So I just added `#define`s on top.


