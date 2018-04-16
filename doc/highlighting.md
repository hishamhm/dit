
Dit syntax highlighting
=======================

Syntax highlighting in Dit is driven by .dithl (Dit Highlighting) files.
Those are installed in `share/dit/highlight` (or `~/.dit/highlight`).

Overview
--------

Dit highlighting files consist of two parts:

* a `FILES` section, which indicate which files are going to be highlighted by this .dithl file;
* and a `RULES` section, which contains the highlighting rules.

The filename of the .dithl file itself is irrelevant.

The `FILES` section
-------------------

Dit runs through the .dithl files trying to find a `FILES` entry that matches the file open in
the current buffer. The order in which .dithl files are searched is not specified. This means
that we assume that the `FILES` entries of different files do not overlap (e.g. if you make two
.dithl files that match `*.c` files, you don't know which you are going to get).

The entries within one `FILES` section are tested in order from top to bottom.

An entry means that an input will be compared against a given string. A rule contains three
words:

* where to get the input
  * `name` for the filename
  * `firstline` to use the first line of the file contents
* how to test it
  * `prefix` to compare against the beginning of the input
  * `suffix` to compare against the end of the input
  * `regex` to run the input through a [POSIX extended regular expression](https://linux.die.net/man/7/regex)
* the string to test the input against

Here are some examples:

```
name suffix .c
name suffix .hs
name prefix Makefile
name prefix makefile
firstline prefix <?xml
firstline regex ^#!.*/[a-z]*sh($|[[:blank:]].*)
```

The `RULES` section
-------------------

The `RULES` section consists of:

* token highlighting rules, which define how to match a token and what color it should be;
* context definitions, which specify groups of rules that apply only in certain parts of the file;
* a few global directives:
  * `insensitive`: A line containing the word `insensitive` by itself states
    that the entire file should be matched in a case-insensitive manner.
  * `script <filename>`: A line containing the word `script` followed by a
    filename indicates a Lua script to be loaded for files of this kind (this is
    not related to syntax highlighting per se, but it means that the script engine
    tailgates on the `FILES` matching of syntax highlighting to determine which
    scripts to run for which files.) 

Contexts
--------

A context is a set of pattern rules that takes into effect once a particular pattern rule
is matched. THe `RULES` section as a whole is an implicit "main context".

A context is declared with the `context` directive in a single line, and needs to be closed with a
`/context` directive in another line. Any rules specified in between these two lines will be
part of the context.

 single  containing two pattern
rules followed by one or three colors:

* the opening pattern rule, which indicates how to enter the context
* the closing pattern rule, which indicates when to exit the context
  * two special pattern rules are recognized only in closing pattern rules:
    * `` `$ `` - meaning the context closes at the end of the line
    * `` `^`$ `` - meaning the context closes in a blank line (which is useful for paragraphs of text)

If one color is listed in the `context` directive, it will be the default color for
the entire context. For example:

```
# brackets and everything inside it will be `bright`
context [ ] bright
/context
```

If three colors are listed in the `context` directive, those will be, respectively,
the color of the opening match, the color of the closing match, and default color
of the context.

```
# brackets will be `brightalt`, contents will be `bright`
context [ ] brightalt brightalt bright
/context
```

Contexts may be nested. Once the closing pattern rule of the context is matched, the
highlight engine reverts back to the enclosing context. Indentation is not relevant.

```
# brackets will be `brightalt`, contents will be `bright`
context [ ] brightalt brightalt bright
    # braces inside brackets will be `brightdiff`, and their contents will be `diff`
    context { } brightdiff brightdiff diff
    /context
/context
```

Includes
--------

Anywhere in the file, you may add an `include` directive. It works as if the contents of the included
file were written directly in the enclosing file. By convention, include files use the .dithlinc
(Dit Highlighting Include) extension, and are searched for in the same directories that .dithl files
are.

Comments
--------

A line starting with `#` is a comment. Comments may be indented, but they must be
at the beginning of the line (i.e. only spaces are allowed before comments).

```
# this is a comment
   # this is a comment too

rule #not_a_comment bright
```

Grammar
-------

```
start :- files_section
       / include

include :- "include" <token> <newline>

files_section :- "FILES" <newline> file_entry*

rules_section :- "RULES" <newline> rule_entry*
         
file_entry :- what_to_test test_mode <token> <newline>
            / include

what_to_test :- "name"
              / "firstline"

test_mode :- "prefix"
            / "suffix"
            / "regex"

rule_entry :- context
            / rule
            / "insensitive" <newline>
            / "script" <token> <newline>
            / include

context :- begin_context rule_entry* end_context

rule :- rule_type match color <newline>

rule_type :- "rule"
           / "eager_rule"
           / "handover_rule"

begin_context :- "context" match close_match color color color <newline>
               / "context" match close_match color <newline>

close_match :- "`$"
             / "`^`$"
             / match

end_context :- "/context" <newline>

match :- ("`^")? match_item+

match_item :- character_match "`+"
            / character_match "`*"
            / character_match "`?"
            / character_match

character_match :- character_set
                 / "``"
                 / "`t"
                 / "`s"
                 / <character>

character_set :- "`[" ("`^")? ( ( <character> | character_sequence ) ("`|")? )+ "`]"

character_sequence :- <character> "`-" <character>

color :- "normal"
       / "bright"
       / "symbol"
       / "brightsymbol"
       / "alt"
       / "brightalt"
       / "diff"
       / "brightdiff"
       / "special"
       / "brightspecial"
       / "specialdiff"
       / "brightspecialdiff"
       / "veryspecial"
       / "dim"

```
