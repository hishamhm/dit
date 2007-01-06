
bin_PROGRAMS = dit

my_dit_C_sources = dit.c Buffer.c CRT.c Clipboard.c DebugMemory.c Field.c \
 FileReader.c FunctionBar.c Hashtable.c Highlight.c Line.c List.c Object.c \
 Panel.c PatternMatcher.c Pool.c RichString.c ScreenManager.c Script.c \
 Stack.c String.c StringBuffer.c TabManager.c Undo.c Vector.c Files.c

# find bindings terminals highlight | grep -v \\.svn
nobase_dist_pkgdata_DATA = bindings/default \
 terminals/linux terminals/xterm-color \
 highlight/autoconf highlight/c highlight/c_mode highlight/css \
 highlight/diff highlight/highlight_rules highlight/html \
 highlight/html_mode highlight/java highlight/latex highlight/lua \
 highlight/make highlight/perl highlight/php highlight/python \
 highlight/shell highlight/xml

dit_SOURCES = $(my_dit_C_sources) config.h debug.h \
 md5.c md5.h Prototypes.h Structures.h

Prototypes.h: $(my_dit_C_sources)
	scripts/GenHeaders Prototypes.h

Structures.h: $(my_dit_C_sources)
	scripts/GenHeaders Structures.h

AM_CFLAGS = -std=c99
AM_CPPFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"