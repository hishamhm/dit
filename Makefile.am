
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = dit

LUA_FLAGS=-DLUA_COMPAT_5_1 -DLUA_USE_LINUX

my_dit_C_sources = dit.c Buffer.c CRT.c Clipboard.c DebugMemory.c Field.c \
 FileReader.c FunctionBar.c Hashtable.c Highlight.c Line.c List.c Object.c \
 Panel.c PatternMatcher.c Pool.c RichString.c Script.c \
 Stack.c String.c StringBuffer.c TabManager.c Undo.c Vector.c Files.c Display.c \
 Text.c lua-compat-5.3/compat-5.3.c
BUILT_SOURCES = Prototypes.h Structures.h

# find bindings terminals highlight scripts -type f -or -type -l | tr '\n' ' ' | fold -s -w 70 | sed 's,$,\\,'
nobase_dist_pkgdata_DATA = \
bindings/default terminals/wsvt25 terminals/xterm-color \
terminals/konsole terminals/linux terminals/rxvt terminals/xterm-1002 \
highlight/conf.dithl highlight/xml.dithl highlight/ltp.dithl \
highlight/shell.dithlinc highlight/lhs.dithl highlight/html.dithl \
highlight/yats.dithl highlight/diff.dithlinc highlight/c.dithl \
highlight/c.dithlinc highlight/yacc.dithl highlight/java.dithl \
highlight/gitcommit.dithl highlight/shellbacktick.dithlinc \
highlight/make.dithl highlight/haskell.dithlinc \
highlight/nestedm4.dithlinc highlight/css.dithl \
highlight/markdown.dithl highlight/haskell.dithl \
highlight/lpeg_re.dithlinc highlight/markdown.dithlinc \
highlight/lua.dithl highlight/shell.dithl \
highlight/javascript.dithlinc highlight/diff.dithl \
highlight/css.dithlinc highlight/autoconf.dithl highlight/php.dithl \
highlight/html.dithlinc highlight/python.dithl highlight/latex.dithl \
highlight/ldoc.dithlinc highlight/latex.dithlinc \
highlight/lua.dithlinc highlight/gitconflict.dithlinc \
highlight/dithl.dithl highlight/lex.dithl highlight/javascript.dithl \
highlight/etlua.dithl highlight/perl.dithl highlight/ruby.dithl \
scripts/cscope.lua scripts/java.lua scripts/c.lua scripts/latex.lua \
scripts/cmd.lua scripts/luacheck/main.lua scripts/luacheck/lexer.lua \
scripts/luacheck/core_utils.lua scripts/luacheck/cache.lua \
scripts/luacheck/whitespace.lua scripts/luacheck/check.lua \
scripts/luacheck/globbing.lua scripts/luacheck/init.lua \
scripts/luacheck/config.lua scripts/luacheck/multithreading.lua \
scripts/luacheck/detect_globals.lua scripts/luacheck/linearize.lua \
scripts/luacheck/reachability.lua scripts/luacheck/utils.lua \
scripts/luacheck/standards.lua scripts/luacheck/expand_rockspec.lua \
scripts/luacheck/inline_options.lua scripts/luacheck/analyze.lua \
scripts/luacheck/options.lua scripts/luacheck/format.lua \
scripts/luacheck/argparse.lua scripts/luacheck/builtin_standards.lua \
scripts/luacheck/filter.lua scripts/luacheck/stds.lua \
scripts/luacheck/parser.lua scripts/luacheck/fs.lua \
scripts/luacheck/version.lua scripts/dit/line_commit.lua \
scripts/dit/code.lua scripts/dit/tab_complete.lua scripts/shell.lua \
scripts/haskell.lua scripts/tmux.lua scripts/lua.lua

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
