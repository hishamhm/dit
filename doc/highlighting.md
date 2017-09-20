
Dit syntax highlighting
=======================

Syntax highlighting in Dit is driven by .dithl (Dit Highlighting) files.
Those are installed in `share/dit/highlight` (or `~/.dit/highlight`).

Grammar
-------

```
start :- files_section
       / include

include :- "include" <token> <newline>

files_section :- "FILES" <newline> file_entry*

rules_section :- "RULES" <newline> rule_entry*
         
file_entry :- what_to_match match_mode <token> <newline>
            / include

what_to_match :- "name"
               / "firstline"

match_mode :- "prefix"
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

begin_context :- "context" context_match context_match color color color <newline>
               / "context" context_match context_match color <newline>

context_match :- match "`$"
               / match

end_context :- "/context" <newline>

match :- ("`^")? match_item+

match_item :- character_set
            / match_item "`+"
            / match_item "`*"
            / match_item "`?"
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
