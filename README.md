<p align="center"><a href="http://hisham.hm/dit"><img border="0" src="http://hisham.hm/dit/dit-white.jpg" alt="Dit"></a></p>

A console text editor for Unix systems that you already know how to use.

http://hisham.hm/dit

Quick reference
---------------

* Ctrl+S - save
* Ctrl+X - cut
* Ctrl+C - copy
* Ctrl+V - paste
* Ctrl+Z or Ctrl+U - undo
* Ctrl+Q or F10 - quit
* Ctrl+F or F3 - find. Inside Find:
  * Ctrl+C - toggle case sensitive
  * Ctrl+W - toggle whole word
  * Ctrl+N - next
  * Ctrl+P - previous
  * Ctrl+R - replace
* Shift-arrows or Alt-arrows - select
  * NOTE! Some terminals "capture" shift-arrow movement for other purposes (switching tabs, etc) and Dit never gets the keys, that's why Dit also tries to support Alt-arrows. Try both and see which one works. I recommend you reconfigure them (you can do this in Konsole setting "Previous Tab" and "Next Tab" to alt-left and alt-right, for example). RXVT-Unicode and Terminology are some terminals that work well out-of-the-box.
* Ctrl+G - go to line
* Ctrl+B - back (to previous location, before last find, go-to-line, tab-switch, etc.)
* Tabs of open files:
  * Ctrl+J - previous tab
  * Ctrl+K - next tab
  * Ctrl+W - close tab
* Ctrl+N - word wrap paragraph
* Ctrl+T - toggle tab mode (Tabs, 2, 3, 4 or 8 spaces - it tries to autodetect based on file contents)

This documentation is incomplete... there are [more keys](https://github.com/hishamhm/dit/blob/master/bindings/default)! Try around!

