
bin_PROGRAMS = dit

my_dit_C_sources = dit.c Buffer.c CRT.c Clipboard.c DebugMemory.c Field.c \
 FileReader.c FunctionBar.c Hashtable.c Highlight.c Line.c List.c Object.c \
 Panel.c PatternMatcher.c Pool.c RichString.c ScreenManager.c Script.c \
 Stack.c String.c StringBuffer.c TabManager.c Undo.c Vector.c Files.c

dit_SOURCES = $(my_dit_C_sources) config.h debug.h \
 md5.c md5.h Prototypes.h Structures.h

Prototypes.h Structures.h: $(my_dit_C_sources)
	scripts/GenHeaders

AM_CFLAGS = -std=c99
AM_CPPFLAGS = -DDATADIR=\"$(datadir)\"

