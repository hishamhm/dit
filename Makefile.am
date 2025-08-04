
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = dit

LUA_FLAGS=-DLUA_COMPAT_5_1 -DLUA_USE_LINUX

my_dit_C_sources = dit.c Buffer.c CRT.c Clipboard.c DebugMemory.c Field.c \
 FileReader.c FunctionBar.c Hashtable.c Highlight.c Line.c List.c Object.c \
 Panel.c PatternMatcher.c Pool.c RichString.c Script.c \
 Stack.c String.c StringBuffer.c TabManager.c Undo.c Vector.c Files.c Display.c \
 Text.c lua-compat-5.3/compat-5.3.c libeditorconfig.a \
 Prototypes.h Structures.h

# find bindings terminals highlight scripts -type f -or -type l | tr '\n' ' ' | fold -s -w 70 | sed 's,$,\\,'
nobase_dist_pkgdata_DATA = \
bindings/default terminals/screen terminals/xterm-256color \
terminals/konsole terminals/wsvt25 terminals/xterm-color \
terminals/rxvt terminals/xterm-1002 terminals/linux \
highlight/conf.dithl highlight/latex.dithlinc \
highlight/javascript.dithlinc highlight/php.dithl \
highlight/autoconf.dithl highlight/proto.dithl highlight/make.dithl \
highlight/lhs.dithl highlight/moonscript.dithl highlight/lp.dithl \
highlight/gitconflict.dithlinc highlight/diff.dithl \
highlight/html.dithlinc highlight/lua.dithl highlight/toml.dithl \
highlight/lua.dithlinc highlight/etlua.dithl highlight/c.dithlinc \
highlight/shell.dithl highlight/ltp.dithl highlight/diff.dithlinc \
highlight/yaml.dithl highlight/yats.dithl \
highlight/shellbacktick.dithlinc highlight/javascript.dithl \
highlight/perl.dithl highlight/rust.dithl highlight/gitcommit.dithl \
highlight/haskell.dithlinc highlight/nestedm4.dithlinc \
highlight/css.dithl highlight/markdown.dithl highlight/ldoc.dithlinc \
highlight/shell.dithlinc highlight/latex.dithl highlight/ruby.dithl \
highlight/json.dithl highlight/html.dithl highlight/python.dithl \
highlight/lpeg_re.dithlinc highlight/dithl.dithl highlight/go.dithl \
highlight/lex.dithl highlight/java.dithl highlight/yacc.dithl \
highlight/c.dithl highlight/t.dithl highlight/css.dithlinc \
highlight/teal.dithl highlight/markdown.dithlinc \
highlight/haskell.dithl highlight/xml.dithl \
scripts/compat53/module.lua scripts/compat53/init.lua scripts/c.lua \
scripts/luacheck/analyze.lua scripts/luacheck/cache.lua \
scripts/luacheck/config.lua scripts/luacheck/version.lua \
scripts/luacheck/globbing.lua scripts/luacheck/parser.lua \
scripts/luacheck/whitespace.lua scripts/luacheck/options.lua \
scripts/luacheck/inline_options.lua scripts/luacheck/utils.lua \
scripts/luacheck/fs.lua scripts/luacheck/linearize.lua \
scripts/luacheck/format.lua scripts/luacheck/core_utils.lua \
scripts/luacheck/filter.lua scripts/luacheck/multithreading.lua \
scripts/luacheck/expand_rockspec.lua scripts/luacheck/check.lua \
scripts/luacheck/standards.lua scripts/luacheck/detect_globals.lua \
scripts/luacheck/argparse.lua scripts/luacheck/main.lua \
scripts/luacheck/builtin_standards.lua scripts/luacheck/stds.lua \
scripts/luacheck/init.lua scripts/luacheck/reachability.lua \
scripts/luacheck/lexer.lua scripts/teal.lua scripts/compat53.lua \
scripts/lua.lua scripts/haskell.lua scripts/json.lua \
scripts/dit/code.lua scripts/dit/notes.lua scripts/dit/config.lua \
scripts/dit/cmd.lua scripts/dit/tab_complete.lua scripts/dit/tmux.lua \
scripts/dit/smart_enter.lua scripts/dit/teal/ttd.lua \
scripts/dit/global.lua scripts/dit/line_commit.lua scripts/shell.lua \
scripts/git_conflict.lua scripts/rust.lua scripts/java.lua \
scripts/go.lua scripts/toml.lua scripts/cscope.lua scripts/latex.lua

EXTRA_DIST = tools/GenHeaders extras/dit.keytab \
   lua-$(DIT_LUA_VERSION)/src/*.[ch] \
   lua-$(DIT_LUA_VERSION)/src/Makefile \
   lua-$(DIT_LUA_VERSION)/README \
   editorconfig/LICENSE \
   doc/* \
   docs/* \
   lua-compat-5.3/compat-5.3.[ch]

dit_SOURCES = $(my_dit_C_sources) config.h debug.h md5.c md5.h
dit_CFLAGS = -std=c99  -DPKGDATADIR=\"$(pkgdatadir)\" -DSYSCONFDIR=\"$(sysconfdir)\" $(LUA_INCLUDE) -rdynamic  -Ieditorconfig/include
dit_LDADD = $(LUA_LIB) libeditorconfig.a
dit_DEPENDENCIES = $(LUA_DEP) libeditorconfig.a

lua-$(DIT_LUA_VERSION)/src/liblua.a:
	cd lua-$(DIT_LUA_VERSION)/src && make CC="$(CC)" MYCFLAGS="$(LUA_FLAGS)" MYLIBS="-Wl,-E -ldl" liblua.a 

noinst_LIBRARIES = libeditorconfig.a

libeditorconfig_a_CFLAGS = -Ieditorconfig/include
libeditorconfig_a_SOURCES = editorconfig/src/config.h editorconfig/src/ec_fnmatch.c editorconfig/src/ec_fnmatch.h \
editorconfig/include/editorconfig/editorconfig.h editorconfig/include/editorconfig/editorconfig_handle.h \
editorconfig/src/editorconfig.c editorconfig/src/editorconfig.h editorconfig/src/editorconfig_handle.c \
editorconfig/src/editorconfig_handle.h editorconfig/src/global.h editorconfig/src/ini.c editorconfig/src/ini.h \
editorconfig/src/misc.c editorconfig/src/misc.h

clean-local:
	rm -f order.txt
	cd lua-$(DIT_LUA_VERSION)/src && make clean

install-data-local:
	$(mkinstalldirs) "$(DESTDIR)$(datadir)/apps/konsole/"
	$(INSTALL_DATA) "$(srcdir)/extras/dit.keytab" "$(DESTDIR)$(datadir)/apps/konsole/dit.keytab"

debug:
	$(MAKE) CFLAGS="-g -O0 $(LUA_FLAGS)"

