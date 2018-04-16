<p align="center"><a href="http://hisham.hm/dit"><img border="0" src="http://hisham.hm/dit/dit-white.jpg" alt="Dit"></a></p>

A console text editor for Unix systems that you already know how to use.

http://hisham.hm/dit

Dependencies
------------

Dit is designed to be light on dependencies. It is developed on Linux,
but it should also be portable to other Unix-like platforms.
Everything you need should be already installed by default in a typical
Linux system.

* ncurses: preferrably newer versions, with Unicode and mouse support
* libiconv: optional, needed for Unicode
* librt: needed for `clock_gettime` on Linux
* bash: used for generating header files at build-time only
* Lua: it bundles Lua 5.3 for scripting so you don't have to worry about
  this dependency, but it can also use the system-installed Lua if
  you have one.

Quick reference
---------------

* Ctrl+Q or F10 - quit
* Ctrl+S - save
* Ctrl+X - cut
* Ctrl+C - copy
* Ctrl+V - paste
* Ctrl+Z or Ctrl+U - undo
* Ctrl+Y - redo
* Shift-arrows or Alt-arrows - select
  * NOTE! Some terminals "capture" shift-arrow movement for other purposes (switching tabs, etc) and Dit never gets the keys, that's why Dit also tries to support Alt-arrows. Try both and see which one works. If Shift-arrows don't work I recommend you reconfigure your terminal (you can do this in Konsole setting "Previous Tab" and "Next Tab" to alt-left and alt-right, for example). RXVT-Unicode and Terminology are some terminals that work well out-of-the-box.
* Ctrl+F or F3 - find. Inside Find:
  * Ctrl+C - toggle case sensitive
  * Ctrl+W - toggle whole word
  * Ctrl+N - next
  * Ctrl+P - previous
  * Ctrl+R - replace
  * Enter - "confirm": exit Find staying where you are
  * Esc - "cancel": exit Find returning to where you started
    * This is useful for "peeking into another part of the file": just Ctrl+F, type something to look, and then Esc to go back to where you were.
* Ctrl+G - go to...
  * ...line number - Type a number to go to a line.
  * ...tab - Type text to go to the open tab that matches that substring.
* Ctrl+B - back (to previous location, before last find, go-to-line, tab-switch, etc.)
  * You can press Ctrl+B multiple times to go back various levels.
* Tabs of open files:
  * Ctrl+J - previous tab
  * Ctrl+K - next tab
  * Ctrl+W - close tab
* Ctrl+N - word wrap paragraph
* Ctrl+T - toggle tab mode (Tabs, 2, 3, 4 or 8 spaces - it tries to autodetect based on file contents)

This documentation is incomplete... there are [more keys](https://github.com/hishamhm/dit/blob/master/bindings/default)! Try around!

