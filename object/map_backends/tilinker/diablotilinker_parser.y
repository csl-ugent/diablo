%{
#include "diablotilinker_parser_extra.h"
%}


/*
 * A simple parser for TiLinker maps... 
 * Very simple, but does the trick...
 */


%token T_LOAD 
%token T_LOAD_ADDRESS 
%token T_STRING
%token T_HEX
%token T_PATTERN
%token T_LEADING_STRING
%token T_SECTION_NAME
%token T_LIB

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
#include <diablotilinker.h>
static char * section_name=NULL;
static char * lib_name=NULL;
t_map * tilinker_parser_map;
int ParseMapTiLinkererror();
int ParseMapTiLinkerlex();

%}
%%

start: program {if (section_name) Free(section_name); section_name=NULL;}
     ;

program: /* Empty */
       | program section
       ;

section: T_LEADING_STRING T_HEX T_HEX T_HEX {if (section_name) Free(section_name); section_name=$1;} section_body {}
       ;

section_body:            		{}
	    | section_body one_body  	{}
	    ;

one_body: T_HEX T_HEX T_STRING T_STRING { t_address addr, size; if (lib_name) { Free(lib_name); lib_name = NULL; } addr = AddressNewForObject(tilinker_parser_map->obj, ((t_uint64)$1.num1) + (((t_uint64)$1.num2) << 32));
				          size = AddressNewForObject(tilinker_parser_map->obj, ((t_uint64)$2.num1) + (((t_uint64)$2.num2) << 32)); 
					    MapInsertNode(tilinker_parser_map, TiLinkerNode($4,section_name,addr,size,$3,NULL));
										}
	| T_HEX T_HEX T_STRING T_LIB T_STRING T_STRING{ t_address addr, size; if (lib_name) { Free(lib_name); } lib_name = StringDup($3); addr = AddressNewForObject(tilinker_parser_map->obj, ((t_uint64)$1.num1) + (((t_uint64)$1.num2) << 32));
					  size = AddressNewForObject(tilinker_parser_map->obj, ((t_uint64)$2.num1) + (((t_uint64)$2.num2) << 32)); 
					    MapInsertNode(tilinker_parser_map, TiLinkerNode($6,section_name,addr,size,$5,$3));
										}

	| T_HEX T_HEX T_LIB T_STRING T_STRING{ t_address addr, size; ASSERT(lib_name, ("Libname not set!")); addr = AddressNewForObject(tilinker_parser_map->obj, ((t_uint64)$1.num1) + (((t_uint64)$1.num2) << 32));
					  size = AddressNewForObject(tilinker_parser_map->obj, ((t_uint64)$2.num1) + (((t_uint64)$2.num2) << 32)); 
					    MapInsertNode(tilinker_parser_map, TiLinkerNode($5,section_name,addr,size,$4,lib_name));
										}

	;

%%

extern long tilinker_map_lexer_line;
int ParseMapTiLinkererror()
{
   printf("Parse error in parsing tilinker style map at  tilinker_map_lexer_line %ld\n",tilinker_map_lexer_line);
   exit(0);
   return 0;
}

