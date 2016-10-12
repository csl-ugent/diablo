%option prefix="LinkerScriptParser"

%{
#define YY_INPUT(buf,result,max_size) \
{ \
  if (linker_script_parser_mode == 0 )\
  {\
    int c = linker_script_parser_buffer[linker_script_parser_offset++]; \
    result = (c == 0) ? YY_NULL : (buf[0] = c, 1); \
  }\
  else\
  {\
    if ( ((result = fread( buf, 1, max_size, yyin )) == 0) \
         && ferror( yyin ) ) \
    YY_FATAL_ERROR( "input in lex scanner failed" ); \
  }\
}

#include <diabloobject.h>
#include "diabloobject_linker_scripts_parser_extra.h"
#include "diabloobject_linker_scripts_parser.h"
#include <diablosupport.h>
#include <string.h>
extern t_object *parser_object;
static char * passon=NULL;
int linker_scripts_lexer_line=1;
static int nests=0;


int linker_script_parser_mode = 0;
char *linker_script_parser_buffer = NULL;
int linker_script_parser_offset = 0;


void lexer_error(char * mesg)
{
  FATAL(("Lexer error at line %d in file: %s",linker_scripts_lexer_line,mesg));
}

void setpasson(char * in)
{
  if (passon) Free(passon);
  passon=StringDup(in);
}

void clearpasson()
{
   if (!passon) lexer_error("No variable/rule name");
   Free(passon);
   passon=NULL;
}

extern t_string last_matched_symbol_name;
extern t_symbol *last_matched_symbol;
int LinkerScriptParserparse();
void LinkerScriptParse(int mode, char * name_or_string, t_object * obj, t_symbol * sym)
{
  if (mode==0)
  {
    linker_script_parser_mode = 0;
    linker_script_parser_buffer = name_or_string;
    linker_script_parser_offset = 0;
  }
  else
  {
    linker_script_parser_mode = 1; 
    LinkerScriptParserin = fopen (name_or_string, "r");
    if (!LinkerScriptParserin)
      FATAL(("Could not open linker script %s", name_or_string));
  }

  if (sym)
  {
    last_matched_symbol=sym;
    if (last_matched_symbol_name) Free(last_matched_symbol_name);
    last_matched_symbol_name=StringDup(SYMBOL_NAME(sym));
  }

  parser_object = obj;
  LinkerScriptParserparse ();

  if (last_matched_symbol_name) Free(last_matched_symbol_name);
  last_matched_symbol_name = NULL;

  if (mode!=0)
  {
    fclose(LinkerScriptParserin);
    LinkerScriptParserin = NULL;
  }
}

%}
%x RULE
%x BODY1
%x BODY2
%x STRING_BODY
%x VARIABLE
%x STRING_VARIABLE
%x COMMENT
%x COMMENT_BODY2
%x COMMENT_RULE
%x LAYOUT_SCRIPT
%%

 /* This lexer performs some grammar checks. They could be done in the parser
  * as well, but than this would complicate the parser. This approach also
  * allows more accurate error reporting */

 /* Initial: see if we have variable definition or rules: if we have a rule we
  * must have VAR = , for RULES we have VAR { */
 
 /* Initial: Newlines can be skipped, we just increase the line counter.*/
"\n"                              { linker_scripts_lexer_line++; }
 /* Initial: The } should be matched inside <RULE> ! */
"}"                               { lexer_error("Unmatched }"); }
 /* Initial: Whitespace can be skipped */
[ \t]                             { }
"/*"				  { BEGIN(COMMENT); }
<COMMENT>"*/"			  { BEGIN(INITIAL); }
<COMMENT>"\n"			  { linker_scripts_lexer_line++; }
<COMMENT>.			  {}
<COMMENT_BODY2>"*/"			  { BEGIN(BODY2); }
<COMMENT_BODY2>"\n"			  { linker_scripts_lexer_line++; }
<COMMENT_BODY2>.			  {}
<COMMENT_RULE>"*/"			  { BEGIN(RULE); }
<COMMENT_RULE>"\n"			  { linker_scripts_lexer_line++; }
<COMMENT_RULE>. 			  {}


 /* Initial: In case we have a rule. We should already have seen a name (stored
  * in passon), otherwise we report an error  */
"{"                               { 
                                    if (!passon) 
                                      lexer_error("Rule started, but no name for rule!"); 
				    LinkerScriptParserlval.string=StringDup(passon);
                                    clearpasson();
                                    BEGIN(RULE); 
				    return LD_RULE_NAME;
                                  } 
 /* Initial: In case we have a variable definition: We should already have seen
  * a name (stored in passon), otherwise we report an error */
=                                 { 
                                    if (!passon) 
                                      lexer_error("Variable definition, but no name for variable"); 
				    LinkerScriptParserlval.string=StringDup(passon);
                                    clearpasson(); 
                                    BEGIN(VARIABLE); 
				    return LD_VARIABLE_DEFINITION;
                                  }
 /* Initial: skip embedded layout scripts (they are parsed separately) */
 SECTIONS			  { BEGIN(LAYOUT_SCRIPT); }

 /* Initial: Match the variable names and rule names: we should not have
  * encountered another name, otherwise throw an error */ 
[a-zA-Z0-9_]*                     { 
                                    if (passon) 
                                      lexer_error("two names, without intervening rule or definition!"); 
                                    setpasson(LinkerScriptParsertext); 
                                  }
 /* Initial: All other characters are unmatched! */ 
.                                 { printf("Found character: '%s'\n",LinkerScriptParsertext); lexer_error("Unmatched character in the INITIAL state"); }

 /* Initial: forks to either <RULE> or <VARIABLE> */

 /* Variable: Matches the value of a variable. For the moment this must be a
  * string, so the only legal values are whitespace, newlines and ". In the
  * future, we could add variables that have a numeric value. A newlines ends
  * the assignment. */

 /* Variable: match the beginning of a string, fork to <STRING_VARIABLE> */
<VARIABLE>"\""                    { BEGIN(STRING_VARIABLE); }
 /* Variable: skip whitespace */
<VARIABLE>[ \t]                   { }
 /* Variable: skip newlines, enter initial state */
<VARIABLE>"\n"                    { linker_scripts_lexer_line++; BEGIN(INITIAL); }
 /* Variable: The rest is illegal */
<VARIABLE>.                       { printf("%s\n",LinkerScriptParsertext); lexer_error("Unmatched character VARIABLE"); }

 /* String variable: Matches a string value for a variable. Strings cannot
  * contain newlines or double quotes (should not be necessary) */

 /* String variable: Newlines in strings are illegal */
<STRING_VARIABLE>"\n"             { lexer_error("Newline in string"); }
 /* String variable: Double quotes end the string */ 
<STRING_VARIABLE>"\""             { BEGIN(VARIABLE);
    LinkerScriptParserlval.string=StringDup(passon); clearpasson(); return LD_VARIABLE_STRING;  }
 /* String variable: Matches the body of the string */
<STRING_VARIABLE>[a-zA-Z \t0-9.$_/]* { if (passon) { printf("%s\n",passon); lexer_error("Strange STRING_VARIABLE"); } setpasson(LinkerScriptParsertext); }
 /* String variable: The rest are illegal strings */
<STRING_VARIABLE>.                { printf("%s\n",LinkerScriptParsertext);lexer_error("Unmatched character STRING_VARIABLE"); }

<RULE>"action"                    { BEGIN(BODY1); return LD_ACTION;  }
<RULE>"trigger"                   { BEGIN(BODY1); return LD_TRIGGER; }
<RULE>"address"                   { BEGIN(BODY1); return LD_ADDRESS; }
<RULE>"symbol"                    { BEGIN(BODY1); return LD_SYMBOL;  }
<RULE>"section"                   { BEGIN(BODY1); return LD_LINKER_SCRIPT_SECTION; }
<RULE>"/*"                        { BEGIN(COMMENT_RULE); }
<RULE>"\n"                        { linker_scripts_lexer_line++; }
<RULE>"}"                         { BEGIN(INITIAL); }
<RULE>[ \t]                       { }
<RULE>"."                         { 
                                    printf("%s\n",LinkerScriptParsertext); 
                                    lexer_error("Unmatched character RULE"); 
                                  }
<BODY1>"{"                        { BEGIN(BODY2); }
<BODY1>"\n"                       { linker_scripts_lexer_line++; }
<BODY1>[ \t]                      { }
<BODY1>.                          { lexer_error("Unmatched character BODY1"); }
<BODY2>"/*"			  { BEGIN(COMMENT_BODY2); }
<BODY2>"\""                       { BEGIN(STRING_BODY); }
<BODY2>"\n"                       { linker_scripts_lexer_line++; }
<BODY2>"("                        { return LD_OPEN_B; }
<BODY2>")"                        { return LD_CLOSE_B;}
<BODY2>"&&"                       { return LD_AND;}
<BODY2>"||"                       { return LD_OR;}
<BODY2>">>"                       { return LD_SHIFT_RIGHT;}
<BODY2>"<<"                       { return LD_SHIFT_LEFT;}
<BODY2>"=="                       { return LD_EQUAL;}
<BODY2>"!="                       { return LD_NOT_EQUAL;}
<BODY2>"+"                        { return LD_ADD;}
<BODY2>"-"                        { return LD_SUB;}
<BODY2>"&"                        { return LD_BITWISE_AND;}
<BODY2>"/"                        { return LD_DIV;}
<BODY2>","                        { return LD_COMMA; }
<BODY2>"!"			  { return LD_NOT; }
<BODY2>"}"                        { BEGIN(RULE);}
<BODY2>"SYMBOL_TYPE_FILE"         {
    LinkerScriptParserlval.string=StringDup("0x1");
    return LD_NAME; }
<BODY2>"SYMBOL_TYPE_FUNCTION"     {
    LinkerScriptParserlval.string=StringDup("0x2");
    return LD_NAME; }
<BODY2>"SYMBOL_TYPE_MARKER"       {
    LinkerScriptParserlval.string=StringDup("0x4");
    return LD_NAME; }
<BODY2>"SYMBOL_TYPE_TLS"          {
    LinkerScriptParserlval.string=StringDup("0x8");
    return LD_NAME; }
<BODY2>"SYMBOL_TYPE_FUNCTION_SMALLCODE"   {
    LinkerScriptParserlval.string=StringDup("0x10");
    return LD_NAME; }
<BODY2>"SYMBOL_TYPE_OBJECT"       {
    LinkerScriptParserlval.string=StringDup("0x20");
    return LD_NAME; }
<BODY2>"SYMBOL_TYPE_NOTYPE"       {
    LinkerScriptParserlval.string=StringDup("0x40");
    return LD_NAME; }
<BODY2>"SYMBOL_TYPE_SECTION"      {
    LinkerScriptParserlval.string=StringDup("0x80");
    return LD_NAME; }
 /* No support for SYMBOL_TYPE_MARK_*, these are automatically added afterwards */
<BODY2>"SYMBOL_TYPE_EXPORT"       {
    LinkerScriptParserlval.string=StringDup("0x1000");
    return LD_NAME; }
<BODY2>"SYMBOL_TYPE_IMPORT"       {
    LinkerScriptParserlval.string=StringDup("0x2000");
    return LD_NAME; }
<BODY2>[A-Za-z0-9_]+[ \t]*"("     {
    LinkerScriptParserlval.string=StringDup(LinkerScriptParsertext);
    LinkerScriptParserlval.string[strlen(LinkerScriptParserlval.string)-1]=0;
    unput('('); return LD_FUNCTION_NAME; }
<BODY2>[A-Za-z0-9_]*              {
    LinkerScriptParserlval.string=StringDup(LinkerScriptParsertext); return
      LD_NAME; }
<BODY2>[ \t]                      { }
<BODY2>.                          { printf("--%s = %d--\n",LinkerScriptParsertext,LinkerScriptParsertext[0]); lexer_error("Unmatched character BODY2"); }
<STRING_BODY>"\n"                 { lexer_error("Newline in string"); }
<STRING_BODY>"\""                 { BEGIN(BODY2);
    LinkerScriptParserlval.string=StringDup(passon); clearpasson(); return LD_BODY_STRING;}
<STRING_BODY>[a-zA-Z \t0-9.$_*+\\&|<>@/=%<{}!:?()-]*    { if (passon) { printf("%s\n",passon); lexer_error("Strange STRING_BODY"); } setpasson(LinkerScriptParsertext);}
<STRING_BODY>.                    { printf("%s\n",LinkerScriptParsertext);        lexer_error("Unmatched character STRING_BODY"); }

 /* rules for skipping embedded layout scripts */
<LAYOUT_SCRIPT>"{"		  { nests++; }
<LAYOUT_SCRIPT>"}"		  { nests--; if (nests == 0) BEGIN(INITIAL); }
<LAYOUT_SCRIPT>.		  { /* discard */ }
<LAYOUT_SCRIPT>\n		  { linker_scripts_lexer_line++; }
%%
int LinkerScriptParserwrap()
{
  return 1;
}
