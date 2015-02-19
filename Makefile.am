
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = dit

my_dit_C_sources = dit.c Buffer.c CRT.c Clipboard.c DebugMemory.c Field.c \
 FileReader.c FunctionBar.c Hashtable.c Highlight.c Line.c List.c Object.c \
 Panel.c PatternMatcher.c Pool.c RichString.c Script.c \
 Stack.c String.c StringBuffer.c TabManager.c Undo.c Vector.c Files.c Display.c \
 Text.c lua-compat-5.3/compat-5.3.c
BUILT_SOURCES = Prototypes.h Structures.h

# find bindings terminals highlight | grep -v \\.svn
nobase_dist_pkgdata_DATA = bindings/default \
 terminals/rxvt terminals/konsole terminals/linux terminals/xterm-color \
 highlight/yacc.dithl highlight/python.dithl highlight/lua.dithl highlight/html.dithl \
 highlight/haskell.dithl highlight/java.dithl highlight/css.dithl highlight/c.dithl \
 highlight/latex.dithl highlight/xml.dithl highlight/autoconf.dithl highlight/lex.dithl \
 highlight/perl.dithl highlight/shell.dithl highlight/dithl.dithl highlight/c.dithlinc \
 highlight/shellbacktick.dithlinc highlight/diff.dithl highlight/php.dithl highlight/html.dithlinc \
 highlight/make.dithl

EXTRA_DIST = tools/GenHeaders extras/dit.keytab lua-$(DIT_LUA_VERSION)/src/*.[ch] lua-$(DIT_LUA_VERSION)/src/Makefile lua-$(DIT_LUA_VERSION)/COPYRIGHT

dit_SOURCES = Prototypes.h Structures.h $(my_dit_C_sources) config.h debug.h md5.c md5.h
dit_CFLAGS = -std=c99  -DPKGDATADIR=\"$(pkgdatadir)\" -DSYSCONFDIR=\"$(sysconfdir)\" $(LUA_INCLUDE)
dit_LDADD = $(LUA_LIB)
dit_DEPENDENCIES = $(LUA_DEP)

lua-$(DIT_LUA_VERSION)/src/liblua.a:
	cd lua-$(DIT_LUA_VERSION)/src && make CC="$(CC)" MYCFLAGS="-DLUA_COMPAT_5_1 -DLUA_USE_LINUX" MYLIBS="-Wl,-E -ldl" liblua.a 

Prototypes.h: $(my_dit_C_sources)
	tools/GenHeaders Prototypes.h

Structures.h: $(my_dit_C_sources)
	tools/GenHeaders Structures.h

clean-local:
	cd lua-$(DIT_LUA_VERSION)/src && make clean

install-data-local:
	$(mkinstalldirs) "$(DESTDIR)$(datadir)/apps/konsole/"
	$(INSTALL_DATA) "$(srcdir)/extras/dit.keytab" "$(DESTDIR)$(datadir)/apps/konsole/dit.keytab"

debug:
	$(MAKE) CFLAGS="-g -O0 -DLUA_USE_LINUX"
