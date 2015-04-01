
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = dit

LUA_FLAGS=-DLUA_COMPAT_5_1 -DLUA_USE_LINUX

my_dit_C_sources = dit.c Buffer.c CRT.c Clipboard.c DebugMemory.c Field.c \
 FileReader.c FunctionBar.c Hashtable.c Highlight.c Line.c List.c Object.c \
 Panel.c PatternMatcher.c Pool.c RichString.c Script.c \
 Stack.c String.c StringBuffer.c TabManager.c Undo.c Vector.c Files.c Display.c \
 Text.c lua-compat-5.3/compat-5.3.c
BUILT_SOURCES = Prototypes.h Structures.h

# find bindings terminals highlight | grep -v \\.svn
nobase_dist_pkgdata_DATA = bindings/default \
 terminals/rxvt terminals/konsole terminals/linux terminals/xterm-color \
 highlight/autoconf.dithl highlight/c.dithl highlight/c.dithlinc \
 highlight/css.dithl highlight/diff.dithl highlight/dithl.dithl \
 highlight/gitcommit.dithl highlight/gitconflict.dithlinc \
 highlight/haskell.dithl highlight/html.dithl highlight/html.dithlinc \
 highlight/java.dithl highlight/latex.dithl highlight/lex.dithl \
 highlight/lua.dithl highlight/make.dithl highlight/nestedm4.dithlinc \
 highlight/perl.dithl highlight/php.dithl highlight/python.dithl \
 highlight/ruby.dithl highlight/shellbacktick.dithlinc highlight/shell.dithl \
 highlight/shell.dithlinc highlight/xml.dithl highlight/yacc.dithl

EXTRA_DIST = tools/GenHeaders extras/dit.keytab lua-$(DIT_LUA_VERSION)/src/*.[ch] lua-$(DIT_LUA_VERSION)/src/Makefile lua-$(DIT_LUA_VERSION)/COPYRIGHT

dit_SOURCES = Prototypes.h Structures.h $(my_dit_C_sources) config.h debug.h md5.c md5.h
dit_CFLAGS = -std=c99  -DPKGDATADIR=\"$(pkgdatadir)\" -DSYSCONFDIR=\"$(sysconfdir)\" $(LUA_INCLUDE) -rdynamic
dit_LDADD = $(LUA_LIB)
dit_DEPENDENCIES = $(LUA_DEP)

lua-$(DIT_LUA_VERSION)/src/liblua.a:
	cd lua-$(DIT_LUA_VERSION)/src && make CC="$(CC)" MYCFLAGS="$(LUA_FLAGS)" MYLIBS="-Wl,-E -ldl" liblua.a 

Prototypes.h: $(my_dit_C_sources)
	tools/GenHeaders Prototypes.h

Structures.h: $(my_dit_C_sources)
	tools/GenHeaders Structures.h

clean-local:
	rm -f order.txt
	cd lua-$(DIT_LUA_VERSION)/src && make clean

install-data-local:
	$(mkinstalldirs) "$(DESTDIR)$(datadir)/apps/konsole/"
	$(INSTALL_DATA) "$(srcdir)/extras/dit.keytab" "$(DESTDIR)$(datadir)/apps/konsole/dit.keytab"

debug:
	$(MAKE) CFLAGS="-g -O0 $(LUA_FLAGS)"
