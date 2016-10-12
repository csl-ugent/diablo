%{
#include <diablosupport.h>
#include <string.h>
#include "diablosupport_dotfile_parser_extra.h"

extern int dot_lexer_line;
extern int yylex (void);
t_dot_graph_elem * parsed_graph=NULL;
t_dot_properties standard={0,NULL,0,0,0,0,NULL,NULL,NULL,NULL,NULL,-1,-1};
void yyerror(const char * in)
{
printf("At line %d: Parse error: %s\n",dot_lexer_line,in);
}
%}

%union 
{
   char * string;
   t_dot_properties proper;
   t_dot_graph_elem * ge;
}



%token DIGRAPH
%token OPEN_C
%token CLOSE_C
%token OPEN_S
%token CLOSE_S
%token DOT_PARSER_NODE
%token DOT_PARSER_GRAPH 
%token DOT_PARSER_SUBGRAPH 
%token DOT_PARSER_EDGE 
%token LABEL 
%token BB 
%token LP 
%token POS 
%token WIDTH 
%token HEIGHT 
%token STYLE 
%token COLOR
%token SHAPE 
%token FILLCOLOR
%token <string> T_ID 
%type <proper> prop;
%type <proper> props;
%type <proper> props_ne;
%type <ge> graph_body;

%%
graph: /* Empty */
     | DIGRAPH T_ID OPEN_C graph_body CLOSE_C 			{ parsed_graph=$4; Free($2); } ;

graph_body: /* Empty */ 			 		{ $$=NULL; } 
	  | DOT_PARSER_NODE OPEN_S props CLOSE_S graph_body 		{ standard=$3; $$=$5; } 
	  | DOT_PARSER_GRAPH OPEN_S props CLOSE_S graph_body 		{ $$=$5; }
	  | T_ID OPEN_S props CLOSE_S graph_body 		{ 
								  t_dot_graph_elem * ng=Malloc(sizeof(t_dot_graph_elem));
								  ng->type=DOT_NODE;
								  ng->select.node.name=$1;
								  ng->select.node.label=$3.label;
								  ng->select.node.color=$3.color;
								  ng->select.node.fillcolor=$3.fillcolor;
								  ng->select.node.style=$3.style;
								  ng->select.node.shape=$3.shape;
								  if ($3.pos)
								  {
								    ng->select.node.x=$3.pos->x;
								    ng->select.node.y=$3.pos->y;
								  }
								  else
								  {
							  	    ng->select.node.x=-1;
								    ng->select.node.y=-1;
								  }
								  ng->select.node.w=$3.width;
								  ng->select.node.h=$3.height;
								  ng->next=$5;
								  $$=ng;
					   			}
	  | T_ID DOT_PARSER_EDGE T_ID  graph_body   	      		{
								  t_dot_graph_elem * ng=Malloc(sizeof(t_dot_graph_elem));
								  ng->type=DOT_EDGE;
								  ng->select.edge.from=$1;
								  ng->select.edge.to=$3;
								  ng->select.edge.pos=NULL;
								  ng->next=$4;
								  $$=ng;
					   			}
	  | T_ID DOT_PARSER_EDGE T_ID OPEN_S props CLOSE_S graph_body 	{ 
								  t_dot_graph_elem * ng=Malloc(sizeof(t_dot_graph_elem));
					                          ng->type=DOT_EDGE;
					                          ng->select.edge.from=$1;
					                          ng->select.edge.to=$3;
								  ng->select.edge.pos=$5.pos;
								  ng->select.edge.dir=$5.dir;
					                          ng->next=$7;
					                          $$=ng;
					                        }; 

props: /* Empty */{ 
     		    $$=standard;
		  } 
     | props_ne   { $$=$1; };

props_ne: prop { $$=$1; }
        | prop props_ne { 
	                        $$=$2;
				if ($1.dir!=0) $$.dir=$1.dir;
				if ($1.label) $$.label=$1.label;
				if ($1.bbx!=-1) $$.bbx=$1.bbx;
				if ($1.bby!=-1) $$.bby=$1.bby;
				if ($1.bbw!=-1) $$.bbw=$1.bbw;
				if ($1.bbh!=-1) $$.bbh=$1.bbh;
				if ($1.color!=NULL) $$.color=$1.color;
				if ($1.style!=NULL) $$.style=$1.style;
				if ($1.shape!=NULL) $$.shape=$1.shape;
				if ($1.fillcolor!=NULL) $$.fillcolor=$1.fillcolor;
				if ($1.pos!=NULL) $$.pos=$1.pos;
				if ($1.height!=-1) $$.height=$1.height;
				if ($1.width!=-1) $$.width=$1.width;
			      }

prop: LABEL T_ID { 
    	              $$=standard;
    	              $$.label=$2; 
		    }
    | BB T_ID    {
    		      char * x=strtok($2,",");
    		      char * y=strtok(NULL,",");
    		      char * w=strtok(NULL,",");
    		      char * h=strtok(NULL,",");
    	              $$=standard;
		      if (!(x && y && w && h)) 
		      {
		      	printf("Incomplete bounding box\n");
			$$.bbx=$$.bby=$$.bbw=$$.bbh=-1;
		      }
		      else
		      {
			$$.bbx=atoi(x);
			$$.bby=atoi(y);
			$$.bbw=atoi(w);
			$$.bbh=atoi(h);
			Free($2);
		      }
                    }
    | POS T_ID   {
		      t_string_array * array;
		      t_dot_pos * pos=NULL;
		      t_string_array_elem * elem;
    	              $$=standard;
		      if (($2[0]=='e') || ($2[0]=='s'))
		      {
		        if ($2[0]=='e') 
			  $$.dir=1; 
			else if ($2[0]=='s') 
			  $$.dir=2;
		      	array=StringDivide($2+2," \t",FALSE,FALSE);
		      }
		      else
		      {
		      	array=StringDivide($2," \t",FALSE,FALSE);
		      }

	              STRING_ARRAY_FOREACH_ELEM(array,elem)
	              {
		        char * x=strtok(elem->string,",");
		        char * y=strtok(NULL,",");
                        if (!(x && y )) 
		        {
		          printf("Incomplete coords %s %s\n",x,y);
		          $$.pos=NULL;
		          break;
		        }
		        else
		        {
		  	  if (!pos)
			  {
			    $$.pos=Malloc(sizeof(t_dot_pos));
			    $$.pos->next=NULL;
			    $$.pos->x=atoi(x);
			    $$.pos->y=atoi(y);
			    pos=$$.pos;
			  }
			  else
			  {
		   	    pos->next=Malloc(sizeof(t_dot_pos));
			    pos=pos->next;
			    pos->x=atoi(x);
			    pos->y=atoi(y);
			    pos->next=NULL;
			  }
		        }
		      }
		      StringArrayFree(array);
		 }
    | WIDTH T_ID {
    	              $$=standard;
		      $$.width=atof($2);
                    }
    | HEIGHT T_ID{
    	              $$=standard;
		      $$.height=atof($2);
                    }
    | COLOR T_ID  {
    	              $$=standard;
		      $$.color=$2;
		    }
    | FILLCOLOR T_ID {
    	              $$=standard;
		      $$.fillcolor=$2;
		    }
    | SHAPE T_ID    {
    		      $$=standard;
	              $$.shape=$2;

    		    }
    | STYLE T_ID    {
    		      $$=standard;
	              $$.style=$2;
    		    }
    | LP T_ID {
    	              $$=standard;
                    };

%%
