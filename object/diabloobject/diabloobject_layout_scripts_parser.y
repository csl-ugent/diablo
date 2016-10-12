%{
#include <string.h>
#include <diabloobject.h>
#include "diabloobject_layout_scripts_parser_extra.h"
extern t_segment * gseg;
int lparselex(void);
static int lparseerror(t_const_string s);

static t_layout_exp * new_exp(int operator_, void * arg1, void * arg2);

static t_layout_rule * last_rule = NULL;
static t_layout_rule * complete_script = NULL;
%}


%union {
  t_string string;
  long long longlong;
  t_layout_exp * layout_exp;
  t_layout_rule * layout_rule;
  t_layout_assign * layout_assign;
  t_layout_secspec * layout_secspec;
}

%type <layout_rule> rule rules command overlay_specs
%type <layout_assign> assign opt_endalign
%type <layout_secspec> section_spec section_name_spec overlay_spec
%type <layout_exp> exp opt_filler opt_address opt_atclause

%token SEGMENT_START
%token SEGMENT_END
%token PUT_REMAINING_SECTIONS
%token SECTIONS
%token ALIGN
%token PROVIDE
%token AT
%token ADDR
%token OVERLAY
%token <string> IDENTIFIER
%token <string> WILDCARD
%token <longlong> NUMBER

%left '|'
%left '&'
%left '+' '-'
%left '*' '/'
%left '~'
%left UMINUS

%start layout_script

%%

layout_script: SECTIONS '{' rules '}' { complete_script = $3; }
             ;

rules: rules rule	{ 
	$$ = $1; 
	ASSERT(last_rule, ("last_rule NULL but not first rule"));
	last_rule->next = $2; 
	while (last_rule->next) last_rule = last_rule->next;
}
     | rule		{ $$ = last_rule = $1;}
     ;

rule: section_spec	{
	t_layout_rule * tmp = Malloc(sizeof(t_layout_rule));
	tmp->kind = SECTION_SPEC;
	tmp->u.secspec = $1;
	tmp->next = NULL;
	$$ = tmp;
}
    | assign ';'	{
 	t_layout_rule * tmp = Malloc(sizeof(t_layout_rule));
 	tmp->kind = ASSIGN;
 	tmp->u.assign = $1;
 	tmp->next = NULL;
 	$$ = tmp;
}
    | command ';'	{
	$$ = $1;
}
    | OVERLAY ':' '{' overlay_specs '}' {
  t_layout_rule *end;
  t_layout_rule *start = Malloc(sizeof(t_layout_rule));
	start->kind = OVERLAY_START;
	start->next = $4;
	
	end = start;
	while (end->next) end = end->next;
	end->next = Malloc(sizeof(t_layout_rule));
	end = end->next;
	end->kind = OVERLAY_END;
	end->next = NULL;

	$$ = start;
}
    ;

overlay_specs: overlay_specs overlay_spec {
	t_layout_rule *rule;
	rule = $1;
	while (rule->next) rule = rule->next;
	rule->next = Malloc(sizeof(t_layout_rule));
	rule = rule->next;

	rule->kind = SECTION_SPEC;
	rule->u.secspec = $2;
	rule->next = NULL;
	$$ = $1;
}
    | overlay_spec {
        t_layout_rule *tmp = Malloc(sizeof(t_layout_rule));
        tmp->kind = SECTION_SPEC;
        tmp->u.secspec = $1;
        tmp->next = NULL;
        $$ = tmp;
}
    ;

overlay_spec: IDENTIFIER '{' '}' {
        t_layout_secspec *tmp = Malloc(sizeof(t_layout_secspec));
        tmp->name = $1;
        tmp->wildcard = FALSE;
	tmp->address = NULL;
	tmp->internal_rule = NULL;
	tmp->filler_exp = NULL;
	$$ = tmp;
}

command: SEGMENT_START '(' '"' IDENTIFIER '"' ')'	{
	t_layout_rule * tmp = Malloc(sizeof(t_layout_rule));
	tmp->kind = SEG_START;
	tmp->u.segment_ident = $4;
	tmp->next = NULL;
	$$ = tmp;
}
    | SEGMENT_END '(' '"' IDENTIFIER '"' ')'		{
	t_layout_rule * tmp = Malloc(sizeof(t_layout_rule));
	tmp->kind = SEG_END;
	tmp->u.segment_ident = $4;
	tmp->next = NULL;
	$$ = tmp;
}
    | PUT_REMAINING_SECTIONS '(' IDENTIFIER ')'	{
	t_layout_rule * tmp = Malloc(sizeof(t_layout_rule));
	tmp->kind = PUT_SECTIONS;

	/* check the validity of the identifier */
	if (!strcmp($3,"CODE_SECTION"))
	{
	  tmp->u.sectype = CODE_SECTION;
	}
	else if (!strcmp($3,"RO_DATA_SECTION"))
	{
	  tmp->u.sectype = RODATA_SECTION;
	}
	else if (!strcmp($3,"DATA_SECTION"))
	{
	  tmp->u.sectype = DATA_SECTION;
	}
	else if (!strcmp($3,"BSS_SECTION"))
	{
	  tmp->u.sectype = BSS_SECTION;
	}
	else if (!strcmp($3,"NOTE_SECTION"))
	{
	  tmp->u.sectype = NOTE_SECTION;
	}
	else
	{
	  FATAL(("not a valid section type identifier: %s",$3));
	}
	Free($3);
	tmp->next = NULL;
	$$ = tmp;
}
    ;

section_spec: section_name_spec opt_address ':' opt_atclause '{' opt_endalign '}' opt_filler	{
	t_layout_secspec *tmp = $1;
	tmp->address = $2;
	tmp->internal_rule = $6;
	tmp->filler_exp = $8;
	$$ = tmp;
}
            ;

section_name_spec: IDENTIFIER	{
	t_layout_secspec * tmp = Malloc(sizeof(t_layout_secspec));
	tmp->name = $1;
	tmp->wildcard = FALSE;
	$$ = tmp;
}
	
                 | WILDCARD	{
	t_layout_secspec * tmp = Malloc(sizeof(t_layout_secspec));
	tmp->name = $1;
	tmp->wildcard = TRUE;
	$$ = tmp;
}
                 ;
opt_address: /* empty */	{ $$ = NULL; }
	   | exp		{ $$ = $1; }
	   ;

opt_atclause: /* empty */	{ $$ = NULL; }
	    | AT '(' exp ')'	{ $$ = $3; }
	    ;

opt_endalign: /* empty */	{ $$ = NULL; }
            | assign ';'	{ $$ = $1; }
	    ;

opt_filler: /* empty */ 	{ $$ = NULL; }
          | '=' exp		{ $$ = $2; }
	  ;

assign: IDENTIFIER '=' exp	{ 
	t_layout_assign * tmp = Malloc(sizeof(t_layout_assign));
	tmp->lhs = $1;
	tmp->rhs = $3;
	tmp->provide = FALSE;
	$$ = tmp;
}
      | PROVIDE '(' IDENTIFIER '=' exp ')'	{
	t_layout_assign * tmp = Malloc(sizeof(t_layout_assign));
	tmp->lhs = $3;
	tmp->rhs = $5;
	tmp->provide = TRUE;
	$$ = tmp;
}
      ;

exp: ALIGN '(' exp ')'		{ $$ = new_exp(ALIGN,$3,NULL); }
   | '(' exp ')'		{ $$ = $2; }
   | '-' exp %prec UMINUS	{ $$ = new_exp(UMINUS,$2,NULL); }
   | '~' exp 			{ $$ = new_exp('~',$2,NULL); }
   | exp '+' exp		{ $$ = new_exp('+',$1,$3); }
   | exp '-' exp		{ $$ = new_exp('-',$1,$3); }
   | exp '*' exp		{ $$ = new_exp('*',$1,$3); }
   | exp '/' exp		{ $$ = new_exp('/',$1,$3); }
   | exp '&' exp 		{ $$ = new_exp('&',$1,$3); }
   | exp '|' exp		{ $$ = new_exp('|',$1,$3); }
   | NUMBER			{ long long tmp = $1; $$ = new_exp(NUMBER,&tmp,NULL); }
   | IDENTIFIER			{ $$ = new_exp(IDENTIFIER,$1,NULL); }
   | ADDR '(' IDENTIFIER ')'	{ $$ = new_exp(ADDR,$3,NULL); }
   ;


%%

static t_layout_exp * new_exp(int operator_, void * arg1, void * arg2)
{
  t_layout_exp * ret = Malloc(sizeof(t_layout_exp));
  ret->operator_ = operator_;
  if (operator_ == NUMBER)
  {
    ret->arg1 = ret->arg2 = NULL;
    ret->identifier = NULL;
    ret->constant = *(long long *)arg1;
  }
  else if (operator_ == IDENTIFIER)
  {
    ret->arg1 = ret->arg2 = NULL;
    ret->constant = 0LL;
    ret->identifier = (t_string)arg1;
  }
  else if (operator_ == ADDR)
  {
    ret->arg1 = ret->arg2 = NULL;
    ret->constant = 0LL;
    ret->identifier = (t_string) arg1;
  }
  else
  {
    ret->arg1 = (t_layout_exp *)arg1;
    ret->arg2 = (t_layout_exp *)arg2;
    ret->identifier = NULL;
    ret->constant = 0LL;
  }
  return ret;
}

extern FILE * lparsein;
int lparselex_destroy  (void);
t_layout_script * LayoutScriptParse(t_const_string scriptfile)
{
  t_layout_script * script=Malloc(sizeof(t_layout_script));
  t_layout_rule * ret;
  FILE * f = fopen(scriptfile,"r");
  if (!f) FATAL(("Could not open %s for reading",scriptfile));
  VERBOSE(0,("Parsing layout script in %s", scriptfile));

  lparsein = f;
  if (lparseparse())
    FATAL(("Parsing appears to have failed"));
  ret = complete_script;
  complete_script = NULL;
  script->first=ret;
  fclose(f);
  lparsein = NULL;
#ifdef DIABLOOBJECT_FLEX_HAS_DESTROY
  lparselex_destroy();
#endif
  return script;
}

/* {{{ rudimentary parse tree printer */
void print_exp(const t_layout_exp * exp)
{
  switch (exp->operator_)
  {
    case ALIGN: 
      printf("align\n"); break;
    case UMINUS:
      printf("uminus\n"); break;
    case IDENTIFIER:
      printf("identifier %s\n", exp->identifier); break;
    case NUMBER:
      printf("constant %llx\n", exp->constant); break;
    default:
      printf("operator %c\n", exp->operator_); break;
  }

  if (exp->arg1)
  {
    printf("subexp 1:\n");
    print_exp(exp->arg1);
  }
  if (exp->arg2)
  {
    printf("subexp 2:\n");
    print_exp(exp->arg2);
  }
}

void print_assign(const t_layout_assign * assign)
{
  printf("assign %s\n", assign->lhs);
  print_exp(assign->rhs);
}

void printscript(void)
{
  t_layout_rule * r = complete_script;
  for (; r; r = r->next)
  {
    printf("<<< rule >>>\n");
    if (r->kind == SECTION_SPEC)
    {
      printf("name %s\n", r->u.secspec->name);
      if (r->u.secspec->internal_rule)
        print_assign(r->u.secspec->internal_rule);
      else
        printf("no internal rule\n");
      if (r->u.secspec->filler_exp)
        print_exp(r->u.secspec->filler_exp);
    }
    else
    {
      print_assign(r->u.assign);
    }
    printf("<<< end rule >>>\n");
  }
}
/* }}} */

extern int layoutlineno;
static int lparseerror(t_const_string s)
{
  FATAL (("parsing failed. parser said: %s at line %d", s, layoutlineno));
  return 0;
}

long long LayoutScriptCalcExp(const t_layout_exp * exp, long long currpos, t_object * obj)
{
  long long res1=0, res2=0;

  if (exp->arg1)
    res1 = LayoutScriptCalcExp(exp->arg1,currpos,obj);
  if (exp->arg2)
    res2 = LayoutScriptCalcExp(exp->arg2,currpos,obj);

  switch (exp->operator_)
  {
    case ALIGN:
      return (currpos + res1 - 1) & ~(res1 - 1); 
    case UMINUS:
      return (-res1);
    case NUMBER:
      return exp->constant;
    case IDENTIFIER:
    {
      if (!strcmp(exp->identifier,"."))
        return currpos;
      else
      {
        if (!strcmp(exp->identifier,"SIZEOF_HEADERS") || !strcmp(exp->identifier,"sizeof_headers"))
        {
          if (gseg)
            SEGMENT_SET_HOIST_HEADERS(gseg,TRUE);
          return OBJECT_OBJECT_HANDLER(obj)->sizeofheaders(obj,OBJECT_LAYOUT_SCRIPT(obj));
        }
        else if (!strcmp(exp->identifier,"LINK_BASE_ADDRESS") || !strcmp(exp->identifier,"link_base_address"))
        {
          return OBJECT_OBJECT_HANDLER(obj)->linkbaseaddress(obj,OBJECT_LAYOUT_SCRIPT(obj));
        }
        else if (!strcmp(exp->identifier,"ALIGN_GOT_AFTER_RELRO") || !strcmp(exp->identifier,"align_got_after_relro"))
        {
          return OBJECT_OBJECT_HANDLER(obj)->aligngotafterrelro(obj,currpos);
        }
        else if (!strcmp(exp->identifier,"ALIGN_DATA_AFTER_RELRO") || !strcmp(exp->identifier,"align_data_after_relro"))
        {
          return OBJECT_OBJECT_HANDLER(obj)->aligndataafterrelro(obj,currpos);
        }
        else if (!strcmp(exp->identifier,"ALIGN_START_OF_RELRO") || !strcmp(exp->identifier,"align_start_of_relro"))
        {
          return OBJECT_OBJECT_HANDLER(obj)->alignstartofrelro(obj,currpos);
        }
        else
          FATAL(("Currently unsupported identifier (%s) in expression",exp->identifier));
      }
    }
    case '~':
      return (~res1);
    case '&':
      return res1 & res2;
    case '|':
      return res1 | res2;
    case '+':
      return res1 + res2;
    case '-':
      return res1 - res2;
    case '*':
      return res1 * res2;
    case '/':
      return res1 / res2;

    default:
      FATAL(("Unknown operator in expression"));
      exit(0);/* Keep the compiler happy */
  }
  return 0; /* Keep the compiler happy */
}

t_layout_rule *LayoutScriptGetRuleForSection(const t_layout_script *script, const t_section *sec)
{
  t_layout_rule *rule;

  for (rule = script->first; rule; rule = rule->next)
  {
    if (rule->kind != SECTION_SPEC) continue;
    if (rule->u.secspec->wildcard)
    {
      if (!StringPatternMatch(rule->u.secspec->name, SECTION_NAME(sec)))
	continue;
    }
    else
    {
      if (strcmp(rule->u.secspec->name, SECTION_NAME(sec)))
	continue;
    }
    return rule;
  }
  return NULL;
}

static void free_exp(const t_layout_exp * exp)
{
  if (!exp) return;

  if (exp->arg1) free_exp(exp->arg1);
  if (exp->arg2) free_exp(exp->arg2);
  if (exp->identifier) Free(exp->identifier);
  Free(exp);
}

static void free_assign(const t_layout_assign * assign)
{
  if (!assign) return;
  Free(assign->lhs);
  free_exp(assign->rhs);
  Free(assign);
}

void LayoutScriptFree(const t_layout_script * script)
{
  t_layout_rule * r, * next;
  for (r=script->first, next=r?r->next:NULL; r; r=next,next=r?r->next:NULL)
  {
    if (r->kind == ASSIGN)
    {
      free_assign(r->u.assign);
    }
    else if (r->kind == SECTION_SPEC)
    {
      free_assign(r->u.secspec->internal_rule);
      free_exp(r->u.secspec->address);
      free_exp(r->u.secspec->filler_exp);
      Free(r->u.secspec->name);
      Free(r->u.secspec);
    }
    else if (r->kind == SEG_START || r->kind == SEG_END)
    {
      Free(r->u.segment_ident);
    }
    else if (r->kind == PUT_SECTIONS)
    {
    }
    Free(r);
  }
  Free(script);
}

/* vim: set shiftwidth=2 foldmethod=marker: */
