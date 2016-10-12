%token NAME 
%token OPTION_TYPE 
%token OPEN_C
%token OR
%token AND 
%token IFSET 
%token IFNOTSET 
%token IF
%token IFNOT
%token CLOSE_C 
%token OPEN_R
%token CLOSE_R 
%token OPTION_DEFAULT 
%token OPTION_REQUIRED
%token OPTION_FORBIDDEN
%token OPTION_REQUIREDTRUE
%token OPTION_FORBIDDENTRUE
%token OPTION_HIDDEN
%token OPTION_GROUP
%token OPTION_SHORT
%token OPTION_LONG
%token OPTION_DESC
%token OPTION_ENV
%token IS 
%token ISEQ 
%token ISNEQ 
%token RET_STRING
%token RET_INT
%token RET_BOOL

%{#include <stdbool.h> %}

%union 
{
   int number;
   char * string;
   bool boolean;
   enum { OPTION_TYPE_STRING, OPTION_TYPE_STRING_ARRAY, OPTION_TYPE_BOOL, OPTION_TYPE_INT, OPTION_TYPE_PATH, OPTION_TYPE_COUNT, OPTION_TYPE_USAGE, OPTION_TYPE_FILE, OPTION_TYPE_VERSION } option_type;
   struct _spec {
     int required;
     int hidden;
     char * modified;
     char * group;
     int nreq;
     char ** reqname;
     int option_type;
     char * name;
     int def_set;
     union { char * c; int i; bool b;} def;
     char * sh;
     char * lo;
     char * desc;
     char * env;
     struct _spec * next;
   } spec;
   struct _spec * speclist;
}
%type <string> NAME;
%type <string> names;
%type <string> inames;
%type <string> modifiers;
%type <string> OPTION_DEFAULT; 
%type <string> RET_STRING; 
%type <number> RET_INT; 
%type <boolean> RET_BOOL;
%type <option_type> OPTION_TYPE; 
%type <spec> option_body;
%type <spec> option_rule;
%type <speclist> options;
%type <spec> prefix_list;
%type <spec> prefix;
%type <spec> prefix_required;
%{

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if STAGE>0
#include "options.h"
#else
#include "../diablosupport/config.h"
#endif

#if STAGE>1
#include "util_options.h"
#endif

FILE * array;
FILE * structure;
FILE * fini;

char * array_c_name;
char * array_h_name;

char * array_name="options";
char * file_name="options";
char * list_name="option_list";
char * fun_name="Options";
char * relative_path=".";
int use_diablosupport=0;
extern int line;
void yyerror(const char * in)
{  
  printf("Parse error on line %d: %s\n",line,in);
#ifdef _MSC_VER
  if (array && array_c_name && !fclose(array))
  {
    if (!remove(array_c_name))
      printf("Removed %s.\n", array_c_name);	
	else
	  printf("Couldn't remove %s.\n", array_c_name);	
  }
  if (structure && array_c_name && !fclose(structure))
  {
    if (!remove(array_h_name))
	{
      printf("Removed %s.\n", array_h_name);	
	  structure = fopen(array_h_name, "w");
	  fclose(structure);
	}
	else
	  printf("Couldn't remove %s.\n", array_h_name);	
  }
#endif
  exit(0);
}

void TableEntry(int required, int hidden, char * group, int type, char * name, struct _spec * spec)
{
fprintf(array,"{%d,",required);
fprintf(array,"%d,",hidden);

if (!group)
fprintf(array,"NULL,");
else
fprintf(array,"\"%s\",",group);


if (!spec->sh)
fprintf(array,"NULL,");
else
fprintf(array,"\"%s\",",spec->sh);

if (!spec->lo)
fprintf(array,"NULL,");
else
fprintf(array,"\"%s\",",spec->lo);

if (!spec->env)
fprintf(array,"NULL,");
else
fprintf(array,"\"%s\",",spec->env);


  switch (type)
	   {
	   case OPTION_TYPE_STRING: 
	   fprintf(array,"OPTION_STRING,");
	   break;
	   case OPTION_TYPE_STRING_ARRAY: 
	   fprintf(array,"OPTION_STRING_ARRAY,");
	   break;
	   case OPTION_TYPE_BOOL:
	   fprintf(array,"OPTION_BOOL,");
	   break;
	   case OPTION_TYPE_INT:
	   fprintf(array,"OPTION_INT,");
	   break;
	   case OPTION_TYPE_PATH:
	   fprintf(array,"OPTION_PATH,");
	   break;
	   case OPTION_TYPE_COUNT:
	   fprintf(array,"OPTION_COUNT,");
	   break;
	   case OPTION_TYPE_USAGE:
	   fprintf(array,"OPTION_USAGE,");
	   break;
	   case OPTION_TYPE_FILE:
	   fprintf(array,"OPTION_FILE,");
	   break;
	   case OPTION_TYPE_VERSION:
	   fprintf(array,"OPTION_VERSION,");
	   break;
	   default:
	   yyerror("unknown type!");
	   }




if (!name) yyerror("No name!");

if (type==OPTION_TYPE_USAGE)
fprintf(array,"NULL, { .function = NULL},");
else if (type==OPTION_TYPE_VERSION)
fprintf(array,"NULL, { .function = %sVersion},",fun_name);
else
{
switch (type)
	   {
	   case OPTION_TYPE_STRING:
		fprintf(array,"&%s.%s_set, { .string =  &%s.%s },",array_name,name,array_name,name);
	   break;
	   case OPTION_TYPE_STRING_ARRAY:
		fprintf(array,"&%s.%s_set, { .sarray =  &%s.%s },",array_name,name,array_name,name);
	   break;
	   case OPTION_TYPE_BOOL:
		fprintf(array,"&%s.%s_set, { .boolean =  &%s.%s },",array_name,name,array_name,name);
	   break;
	   case OPTION_TYPE_INT:
		fprintf(array,"&%s.%s_set, { .sint =  &%s.%s},",array_name,name,array_name,name);
	   break;
	   case OPTION_TYPE_PATH:
		fprintf(array,"&%s.%s_set, { .path =  &%s.%s},",array_name,name,array_name,name);
	   break;
	   case OPTION_TYPE_COUNT:
		fprintf(array,"&%s.%s_set, { .count = &%s.%s},",array_name,name,array_name,name);
	   break;
	   case OPTION_TYPE_USAGE:
		fprintf(array,"&%s.%s_set, &%s.%s,",array_name,name,array_name,name);
	   break;
	   case OPTION_TYPE_FILE:
		fprintf(array,"&%s.%s_set, { .string =  &%s.%s},",array_name,name,array_name,name);
	   break;
	   case OPTION_TYPE_VERSION:
		fprintf(array,"&%s.%s_set, {.function = &%s.%s},",array_name,name,array_name,name);
	   break;
	   default:
	   yyerror("unknown type!");
	   }
}



switch (spec->def_set)
{
case 0:
fprintf(array,"{ .default_string = \"%s\"},",spec->def.c);
break;
case 1:
fprintf(array,"{ .default_count = %d},",spec->def.i);
break;
case 2:
fprintf(array,"{ .default_string = %s},",spec->def.c);
break;
case 3:
fprintf(array,"{ .default_bool = %s},",spec->def.b?"TRUE":"FALSE");
break;
default:
       fprintf(array,"{NULL},");
}

if (!spec->desc)
fprintf(array,"\"\"},\n");
else
fprintf(array,"\"%s\"},\n",spec->desc);



}

int yylex();
%}
%%

program: options { 
       int count=0;
	   int noopt=1;
       unsigned long tel;
       char * file_name_clean=strdup(file_name);
       struct _spec * list=$1;
       
       fprintf(array,"#include \"%s.h\"\n"
                     "#ifdef __cplusplus\nextern \"C\" {\n#endif\n"
                     "extern void %sVersion();\n"
                     "#ifdef __cplusplus\n}\n#endif\n"
                     "t_option %s[]={\n",file_name,fun_name,list_name);
       while(list)
       {
	  TableEntry(list->required,list->hidden,list->group, list->option_type,list->name,list);
          count++;
          list=list->next;
       }
       fprintf(array,"{0,0,NULL,NULL,NULL,NULL,OPTION_NONE,NULL,{NULL},{NULL},NULL}\n};\n");

       list=$1;

	for (tel=0; tel<strlen(file_name_clean); tel++)
		if (file_name_clean[tel]=='/') file_name_clean[tel]='_';
  
       fprintf(structure, "#ifdef __cplusplus\nextern \"C\" {\n#endif\n");

       fprintf(structure,"#ifndef OPTION_STRUCT_%s\n#define OPTION_STRUCT_%s\n#include <stdlib.h>\n",file_name_clean,file_name_clean);
       if (use_diablosupport) 
         fprintf(structure,"#include <diablosupport.h>\n");
       else
         fprintf(structure,"#include \"%s/opt_gen_handler.h\"\n",relative_path);        
       
       while(list)
       {           
		   if ((list->option_type!=OPTION_TYPE_USAGE) && (list->option_type!=OPTION_TYPE_VERSION))
	   {
	      /** The structure typedef must only be written if it contains members. */		   
		   if (noopt == 1)
		   {
		     fprintf(structure,"typedef struct {\n");
			 noopt = 0;
		   }
	      fprintf(structure,"t_bool %s_set;\n",list->name);
	      switch (list->option_type)
	      {
	         case OPTION_TYPE_STRING_ARRAY:
	           fprintf(structure,"t_string_array * ");
	           break;

	         case OPTION_TYPE_STRING:
	         case OPTION_TYPE_FILE:
	           fprintf(structure,"t_string ");
	           break;

	         case OPTION_TYPE_BOOL:
	           fprintf(structure,"t_bool ");
	           break;
	   
	         case OPTION_TYPE_INT:
	           fprintf(structure,"t_int32 ");
	           break;
	         
		 case OPTION_TYPE_PATH:
	           fprintf(structure,"t_path * ");
	           break;
	   
	         case OPTION_TYPE_COUNT:
	           fprintf(structure,"t_uint32 ");
	           break;
	      }
	      fprintf(structure,"%s;\n",list->name);
	   }

          list=list->next;
       }
	   if (noopt == 0)
         fprintf(structure,"} t_%s;\nextern t_%s %s;\n",array_name,array_name,array_name);
       fprintf(structure,"extern t_option %s[];\n",list_name);
       fprintf(structure,"void %sInit();\nvoid %sVerify();\nvoid %sFini();\n#endif\n",fun_name,fun_name,fun_name); 
       fprintf(structure, "#ifdef __cplusplus\n}\n#endif\n");
       
       list=$1;
	   if (noopt == 0)
         fprintf(array,"t_%s %s;\nvoid %sInit()\n{\n",array_name, array_name,fun_name);
	   else
	     fprintf(array,"void %sInit()\n{", fun_name);
       while(list)
       {
          if ((list->option_type!=OPTION_TYPE_USAGE) && (list->option_type!=OPTION_TYPE_VERSION))
          {
             fprintf(array,"%s.%s_set=0;\n",array_name,list->name);
          }
          list=list->next;
       }
       fprintf(array,"}\n");
       list=$1;
  
       fprintf(array,"void %sVerify()\n{\n", fun_name);
       while(list)
       {
          if ((list->option_type!=OPTION_TYPE_USAGE) && (list->option_type!=OPTION_TYPE_VERSION))
	  if ((list->required)&&(list->modified)) fprintf(array,"if %s\n",list->modified); 
	  if (list->required==1) fprintf(array,"if (!%s.%s_set) FATAL((\"required option %s not set\"));\n",array_name,list->name,list->sh?list->sh:list->lo?list->lo:list->name);
	  else if (list->required==2) fprintf(array,"if (%s.%s_set) FATAL((\"forbidden option %s set\"));\n",array_name,list->name,list->sh?list->sh:list->lo?list->lo:"(no switch)");
	  else if (list->required==3) fprintf(array,"if (!%s.%s) FATAL((\"requiredtrue option %s is false\"));\n",array_name,list->name,list->sh?list->sh:list->lo?list->lo:"(no switch)");
	  else if (list->required==4) fprintf(array,"if (%s.%s) FATAL((\"forbiddentrue option %s true\"));\n",array_name,list->name,list->sh?list->sh:list->lo?list->lo:"(no switch)");
          list=list->next;
       }
       fprintf(array,"}\n");


       list=$1;
       fprintf(array,"void %sFini()\n{\n",fun_name);
       while(list)
       {
          if ((list->option_type==OPTION_TYPE_STRING)
          || (list->option_type==OPTION_TYPE_FILE))
	  {
	  	fprintf(array,"if (%s.%s_set) Free(%s.%s);\n",array_name,list->name,array_name,list->name);
	  }
          else if (list->option_type == OPTION_TYPE_STRING_ARRAY)
          {
	  	fprintf(array,"if (%s.%s_set) StringArrayFree(%s.%s);\n", array_name, list->name, array_name, list->name);
          }
          else if (list->option_type==OPTION_TYPE_PATH)
	  {
	  	fprintf(array,"if (%s.%s_set) PathFree(%s.%s);\n",array_name,list->name,array_name,list->name);
	  }
          list=list->next;
       }
       fprintf(array,"}\n");

       printf("Found %d options in file\n",count);
       }
       ;

options: option_rule { $$=malloc(sizeof(struct _spec)); memcpy($$,&$1,sizeof(struct _spec)); $$->next=NULL; }
       | options option_rule { struct _spec * next=$1; $$=malloc(sizeof(struct _spec)); memcpy($$,&$2,sizeof(struct _spec)); $$->next=next; }
       ;



prefix_list: /* None */ { $$.hidden=0; $$.required=0; $$.modified=NULL; $$.group=NULL; }
	   | prefix_list prefix { if ($2.hidden) $$.hidden=$2.hidden; else $$.hidden=$1.hidden; 
	                          if ($2.required) $$.required=$2.required; else $$.required=$1.required; 
	                          if ($2.modified) $$.modified=$2.modified; else $$.modified=$1.modified; 
	                          if ($2.group) $$.group=$2.group; else $$.group=$1.group; 
	   			}
	   ;

prefix:  prefix_required { $$.hidden=0; $$.required=$1.required; $$.modified=$1.modified;$$.group=NULL;}
      | OPTION_HIDDEN {$$.hidden=1; $$.required=0; $$.modified=NULL; $$.group=NULL;}
      | OPTION_GROUP RET_STRING { $$.hidden=0; $$.required=0; $$.modified=NULL;$$.group=$2; } 
      ;


prefix_required: OPTION_REQUIRED modifiers {$$.required=1; $$.modified=$2; }
	       | OPTION_FORBIDDEN modifiers {$$.required=2; $$.modified=$2; } 
	       | OPTION_REQUIREDTRUE modifiers {$$.required=3; $$.modified=$2; } 
	       | OPTION_FORBIDDENTRUE modifiers {$$.required=4; $$.modified=$2; } 
	       ;

modifiers: /* None*/ { $$=NULL; }
	 | IFSET OPEN_R names CLOSE_R { $$=malloc(strlen($3)+ strlen("()") +1); sprintf($$,"(%s)",$3);  }
	 | IFNOTSET OPEN_R names CLOSE_R { $$=malloc(strlen($3)+ strlen("(!())") +1); sprintf($$,"(!(%s))",$3);  }
	 | IF OPEN_R inames CLOSE_R { $$=malloc(strlen($3)+ strlen("()") +1); sprintf($$,"(%s)",$3);  }
	 | IFNOT OPEN_R inames CLOSE_R { $$=malloc(strlen($3)+ strlen("(!())") +1); sprintf($$,"(!(%s))",$3);  }
	 ;

names: NAME { $$=malloc(strlen($1)+strlen("._set")+strlen(array_name) + 1); sprintf($$,"%s.%s_set",array_name,$1); free($1); }
     | names OR NAME { $$=malloc(strlen($1)+strlen($3)+strlen(array_name) + strlen("._set") + strlen("||") + 1); sprintf($$,"%s||%s.%s_set",$1,array_name,$3);}
     | names AND NAME { $$=malloc(strlen($1)+strlen($3)+strlen(array_name) + strlen("._set") + strlen("&&") + 1); sprintf($$,"%s&&%s.%s_set",$1,array_name,$3);}
     | OPEN_R names CLOSE_R { yyerror("Implement (");}
     ;

inames: RET_INT {   $$=malloc(20); sprintf($$,"(%d)",$1); }
     | NAME { $$=malloc(strlen($1)+strlen(".")+strlen(array_name) + 1); sprintf($$,"%s.%s",array_name,$1); free($1); }
     | inames OR inames { $$=malloc(strlen($1)+strlen($3)+strlen(array_name) + strlen(".") + strlen("||") + 1); sprintf($$,"(%s)||(%s)",$1,$3);}
     | inames AND inames { $$=malloc(strlen($1)+strlen($3)+strlen(array_name) + strlen(".") + strlen("&&") + 1); sprintf($$,"(%s)&&(%s)",$1,$3);}
     | inames ISEQ inames { $$=malloc(strlen($1)+strlen($3)+strlen(array_name) + strlen(".") + strlen("&&") + 1); sprintf($$,"(%s)==(%s)",$1,$3);}
     | inames ISNEQ inames { $$=malloc(strlen($1)+strlen($3)+strlen(array_name) + strlen(".") + strlen("&&") + 1); sprintf($$,"(%s)!=(%s)",$1,$3);}
     | OPEN_R inames CLOSE_R { $$=$2; }
     ;



option_rule: prefix_list OPTION_TYPE NAME OPEN_C option_body CLOSE_C { $$=$5; $$.required=$1.required; $$.modified=$1.modified; $$.hidden=$1.hidden; $$.option_type=$2; $$.group=$1.group; $$.name=$3; }
	   ;

option_body: { $$.sh=$$.lo=$$.def.c=$$.desc=$$.env=$$.modified=NULL; $$.def_set=-1; }  /* Empty */
	   | option_body OPTION_DEFAULT IS RET_STRING { $$=$1; $$.def.c=$4; $$.def_set=0; } 
	   | option_body OPTION_DEFAULT IS NAME { $$=$1; $$.def.c=$4; $$.def_set=2; } 
	   | option_body OPTION_DEFAULT IS RET_INT { $$=$1; $$.def.i=$4; $$.def_set=1; } 
     | option_body OPTION_DEFAULT IS RET_BOOL { $$=$1; $$.def.b=$4; $$.def_set=3; }
	   | option_body OPTION_SHORT IS RET_STRING { $$=$1; $$.sh=$4; } 
	   | option_body OPTION_LONG IS RET_STRING { $$=$1; $$.lo=$4; } 
	   | option_body OPTION_DESC IS RET_STRING { $$=$1; $$.desc=$4; } 
	   | option_body OPTION_ENV IS RET_STRING { $$=$1; $$.env=$4; } 
	   ;
%%

extern FILE * yyin;

#if STAGE>0 
extern t_option option_list[];
void OptGenParseCommandLine (t_option * option_list, t_uint32 argc, char **argv, t_bool final);
#endif

int main(int argc, char ** argv)
{ 
  array_c_name = array_h_name = NULL;
  array = structure = NULL;
#if STAGE>0
  OptionsInit();
  OptGenParseCommandLine(option_list,argc,argv,1);
  if (options.inname_set)
  {
    printf("using input file %s\n",options.inname);
    yyin=fopen(options.inname,"r");
    if (!yyin) yyerror("input file not found");
  }

  if (options.outname_set)
  {
     array_name=options.outname;
     file_name=options.outname;
  }

  if (options.arrayname_set)
  {
     array_name=options.arrayname;
  }

  if (options.listname_set)
  {
     list_name=options.listname;
  }

  if (options.funname_set)
  {
     fun_name=options.funname;
  }


  if (options.relpath_set)
  {
     relative_path=options.relpath;
  }

  if (options.diablosupport_set)
  {
     use_diablosupport=options.diablosupport;
  }
#else
#ifdef USE_ARGV_1 /* To facilitate CMake because then I don't have to pipe stdin somehow */
    yyin=fopen(argv[1], "r");
#endif
#endif

  array_c_name=malloc(strlen(file_name)+strlen(".c")+1);
  array_h_name=malloc(strlen(file_name)+strlen(".h")+1);

  sprintf(array_c_name,"%s%s",file_name,".c");
  sprintf(array_h_name,"%s%s",file_name,".h");
  array=fopen(array_c_name,"w");
  structure=fopen(array_h_name,"w");
  yyparse();
  fclose(array);
  fclose(structure);
  return 0;
}
