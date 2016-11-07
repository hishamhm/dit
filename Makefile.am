
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = dit

LUA_FLAGS=-DLUA_COMPAT_5_1 -DLUA_USE_LINUX

my_dit_C_sources = dit.c Buffer.c CRT.c Clipboard.c DebugMemory.c Field.c \
 FileReader.c FunctionBar.c Hashtable.c Highlight.c Line.c List.c Object.c \
 Panel.c PatternMatcher.c Pool.c RichString.c Script.c \
 Stack.c String.c StringBuffer.c TabManager.c Undo.c Vector.c Files.c Display.c \
 Text.c lua-compat-5.3/compat-5.3.c
BUILT_SOURCES = Prototypes.h Structures.h

# find bindings terminals highlight scripts -type f | tr '\n' ' ' | fold -s -w 70 | sed 's,$,\\,'
nobase_dist_pkgdata_DATA = \
bindings/default terminals/rxvt terminals/xterm-color \
terminals/konsole terminals/wsvt25 terminals/linux \
highlight/gitcommit.dithl highlight/ltp.dithl highlight/ldoc.dithlinc \
highlight/python.dithl highlight/css.dithl highlight/c.dithlinc \
highlight/gitconflict.dithlinc highlight/nestedm4.dithlinc \
highlight/java.dithl highlight/autoconf.dithl highlight/conf.dithl \
highlight/make.dithl highlight/diff.dithl highlight/php.dithl \
highlight/dithl.dithl highlight/shell.dithlinc highlight/html.dithl \
highlight/shell.dithl highlight/ruby.dithl highlight/perl.dithl \
highlight/yacc.dithl highlight/lua.dithlinc highlight/markdown.dithl \
highlight/lex.dithl highlight/shellbacktick.dithlinc \
highlight/haskell.dithl highlight/c.dithl highlight/latex.dithl \
highlight/diff.dithlinc highlight/lua.dithl highlight/xml.dithl \
highlight/html.dithlinc highlight/etlua.dithl scripts/lua.lua \
scripts/haskell.lua scripts/shell.lua scripts/latex.lua \
scripts/luacheck/argparse.lua scripts/luacheck/check.lua \
scripts/luacheck/linearize.lua scripts/luacheck/inline_options.lua \
scripts/luacheck/lexer.lua scripts/luacheck/options.lua \
scripts/luacheck/format.lua scripts/luacheck/expand_rockspec.lua \
scripts/luacheck/analyze.lua scripts/luacheck/reachability.lua \
scripts/luacheck/utils.lua scripts/luacheck/init.lua \
scripts/luacheck/parser.lua scripts/luacheck/core_utils.lua \
scripts/luacheck/filter.lua scripts/luacheck/stds.lua scripts/cmd.lua \
scripts/dit/code.lua scripts/dit/tab_complete.lua \
scripts/dit/line_commit.lua scripts/cscope.lua scripts/c.lua \
scripts/java.lua

EXTRA_DIST = tools/GenHeaders extras/dit.keytab \
   lua-$(DIT_LUA_VERSION)/src/*.[ch] \
   lua-$(DIT_LUA_VERSION)/src/Makefile \
   lua-$(DIT_LUA_VERSION)/README \
   editorconfig/LICENSE \
   lua-compat-5.3/compat-5.3.[ch]

dit_SOURCES = Prototypes.h Structures.h $(my_dit_C_sources) config.h debug.h md5.c md5.h
dit_CFLAGS = -std=c99  -DPKGDATADIR=\"$(pkgdatadir)\" -DSYSCONFDIR=\"$(sysconfdir)\" $(LUA_INCLUDE) -rdynamic  -Ieditorconfig/include
dit_LDADD = $(LUA_LIB) libeditorconfig.a
dit_DEPENDENCIES = $(LUA_DEP)

lua-$(DIT_LUA_VERSION)/src/liblua.a:
	cd lua-$(DIT_LUA_VERSION)/src && make CC="$(CC)" MYCFLAGS="$(LUA_FLAGS)" MYLIBS="-Wl,-E -ldl" liblua.a 

noinst_LIBRARIES = libeditorconfig.a

libeditorconfig_a_CFLAGS = -Ieditorconfig/include
libeditorconfig_a_SOURCES = editorconfig/src/config.h editorconfig/src/ec_fnmatch.c editorconfig/src/ec_fnmatch.h \
editorconfig/include/editorconfig/editorconfig.h editorconfig/include/editorconfig/editorconfig_handle.h \
editorconfig/src/editorconfig.c editorconfig/src/editorconfig.h editorconfig/src/editorconfig_handle.c \
editorconfig/src/editorconfig_handle.h editorconfig/src/global.h editorconfig/src/ini.c editorconfig/src/ini.h \
editorconfig/src/misc.c editorconfig/src/misc.h

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



