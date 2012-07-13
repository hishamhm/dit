
bin_PROGRAMS = dit

my_dit_C_sources = dit.c Buffer.c CRT.c Clipboard.c DebugMemory.c Field.c \
 FileReader.c FunctionBar.c Hashtable.c Highlight.c Line.c List.c Object.c \
 Panel.c PatternMatcher.c Pool.c RichString.c ScreenManager.c Script.c \
 Stack.c String.c StringBuffer.c TabManager.c Undo.c Vector.c Files.c Display.c
BUILT_SOURCES = Prototypes.h Structures.h

# find bindings terminals highlight | grep -v \\.svn
nobase_dist_pkgdata_DATA = bindings/default \
 terminals/linux terminals/xterm-color terminals/konsole terminals/rxvt \
 highlight/autoconf highlight/c highlight/c_mode highlight/css \
 highlight/diff highlight/highlight_rules highlight/html \
 highlight/html_mode highlight/java highlight/latex highlight/lua \
 highlight/make highlight/perl highlight/php highlight/python \
 highlight/shell highlight/xml

EXTRA_DIST = tools/GenHeaders extras/dit.keytab lua-$(DIT_LUA_VERSION)/src/*.[ch] lua-$(DIT_LUA_VERSION)/src/Makefile lua-$(DIT_LUA_VERSION)/COPYRIGHT

dit_SOURCES = Prototypes.h Structures.h $(my_dit_C_sources) config.h debug.h md5.c md5.h
dit_CFLAGS = -std=c99  -DPKGDATADIR=\"$(pkgdatadir)\" -DSYSCONFDIR=\"$(sysconfdir)\" $(LUA_INCLUDE)
dit_LDADD = $(LUA_LIB)
dit_DEPENDENCIES = $(LUA_DEP)

lua-$(DIT_LUA_VERSION)/src/liblua.a:
	cd lua-$(DIT_LUA_VERSION)/src && make MYCFLAGS=-DLUA_USE_LINUX MYLIBS="-Wl,-E -ldl" liblua.a 

Prototypes.h: $(my_dit_C_sources)
	tools/GenHeaders Prototypes.h

Structures.h: $(my_dit_C_sources)
	tools/GenHeaders Structures.h

clean-local:
	cd lua-$(DIT_LUA_VERSION)/src && make clean

install-data-local:
	$(mkinstalldirs) "$(DESTDIR)$(datadir)/apps/konsole/"
	$(INSTALL_DATA) "$(srcdir)/extras/dit.keytab" "$(DESTDIR)$(datadir)/apps/konsole/dit.keytab"
