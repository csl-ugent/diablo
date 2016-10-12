%{
#include "diablobinutils_parser_extra.h"
%}


/*
 * A simple parser for Binutils maps... 
 * Very simple, but does the trick...
 */


%token T_LOAD 
%token T_LOAD_ADDRESS 
%token T_STRING
%token T_HEX
%token T_PATTERN
%token T_LEADING_STRING
%token T_SECTION_NAME
%token T_IS
%token T_BUILTIN

%union 
{
	long num;  /* Don't try to use long longs inhere .. Bison bug*/
	struct 
	{
		unsigned long num1;
		unsigned long num2;
	} ll;
	char * string;
}

%type <ll> T_HEX 
%type <string> T_STRING 
%type <string> T_LEADING_STRING
%type <string> T_SECTION_NAME


%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <diablobinutils.h>
static char * section_name=NULL;
t_map * binutils_parser_map;
int ParseMapBinutilserror();
int ParseMapBinutilslex();

t_uint64 section_address=0;
t_uint64 section_load_address=0;
static t_section * section =NULL;
static t_uint64 parent_section_size=0;
static t_bool skip_empty_section = FALSE;
%}
%%

start: program {if (section_name) Free(section_name); section_name=NULL;}
     ;

program: /* Empty */
       | program unary 
       ;

unary: load { }
     | section { }
     ;

load: T_LOAD T_STRING     { Free($2); }
    | T_LOAD T_BUILTIN T_STRING	{ Free($3); }
    ;

section: T_LEADING_STRING {}
       | T_LEADING_STRING {
        if (section_name) Free(section_name); section_name=$1; skip_empty_section = FALSE;
       } section_body {}
       | T_LEADING_STRING T_HEX {
	if (section_name) Free(section_name);
	section_name=$1;
	skip_empty_section = FALSE;
	section=SectionGetFromObjectByName(binutils_parser_map->obj,section_name);
	section_load_address = section_address = ((t_uint64)$2.num1) + (((t_uint64)$2.num2) << 32);
       } T_HEX {
	parent_section_size = ((t_uint64)$4.num1) + (((t_uint64)$4.num2) << 32);
	if (!parent_section_size) {skip_empty_section = TRUE;}
       } section_body_can_be_empty {}	
       ;

section_body_can_be_empty: 		{}
			 | section_body {}
			 ;

section_body:  one_body			{}
	    | section_body one_body 	{}
	    ;

one_body: T_LOAD_ADDRESS T_HEX {
	section_load_address = ((t_uint64)$2.num1) + (((t_uint64)$2.num2) << 32);
	if (!section && parent_section_size)
		FATAL(("Section %s not found",section_name));
	else if (section) {
		SECTION_SET_LOAD_ADDRESS(section,AddressNewForObject(binutils_parser_map->obj,
							section_load_address));
	}
}
	| matched { }
	| T_PATTERN { }
	;

matched: T_SECTION_NAME T_HEX T_HEX T_STRING  { 
         if ((strncmp($1,".gnu.warning",12)!=0) 
         && ((strncmp($1,".got",4)!=0) ||
             (strcmp($1,".got2")==0)) /* .got2 needed for ppc32 */
         && (strncmp($1,".fpc",4)!=0) 
         && (strncmp($1,".glue_7",7)!=0)
         && (strncmp($1,".glue_7t",8)!=0) /* already caught by the above, just to show it's intentional */
         && (strncmp($1,".vfp11_veneer",13)!=0)
         && (strncmp($1,".v4_bx",6)!=0)
         && (strncmp(section_name,".comment",8)!=0) 
         && (strcmp(section_name,".note")!=0) 
         && (strncmp(section_name,".stab",5)!=0) 
         && (strncmp(section_name,".debug",6)!=0) 
         && (strncmp(section_name,".reginfo",8)!=0) 
         && (strncmp(section_name,".pdr",4)!=0)
         && !skip_empty_section
         ) { 
	   t_address addr =
	     AddressNewForObject(binutils_parser_map->obj,
	       ((t_uint64)$2.num1) + (((t_uint64)$2.num2) << 32));
	   t_address size =
	     AddressNewForObject(binutils_parser_map->obj,
	       ((t_uint64)$3.num1) + (((t_uint64)$3.num2) << 32));
	   MapInsertNode(binutils_parser_map, BinutilsNode($1,section_name,addr,size,$4,FALSE));
	 } /* Insert in the avl-tree */  
	 /* free the string values allocated in the lexer */
	 Free($1);
	 Free($4);
       }
       | matched T_HEX T_STRING { Free($3); } /* Symbol definition */
       | matched T_HEX T_STRING T_STRING{ Free($3); } /* Symbol definition with param, e.g.  bool a() */
       | T_SECTION_NAME T_HEX T_HEX T_BUILTIN T_STRING {
	 t_address addr =
	   AddressNewForObject(binutils_parser_map->obj,
	     ((t_uint64)$2.num1) + (((t_uint64)$2.num2) << 32));
	 t_address size =
	   AddressNewForObject(binutils_parser_map->obj,
	     ((t_uint64)$3.num1) + (((t_uint64)$3.num2) << 32));
	 MapInsertNode(binutils_parser_map, BinutilsNode($1,section_name,addr,size,$5,TRUE));
       }
       ;

%%

extern long line;
int ParseMapBinutilserror()
{
   FATAL(("Parse error in parsing binutils style map at line %ld",line));
}

