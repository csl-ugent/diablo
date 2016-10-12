/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/** \file
 *
 * Apart from combining the different sections specified in the input
 * objectfiles, linkers also define symbols (often called linker symbols),
 * create special sections,... This functionality is linker- and
 * architecture-specific. To describe this functionality for different
 * linker/architecture combinations diabloobjects uses "linkerscripts". This
 * file holds the typedefs, types, function, and defines needed to parse and
 * interpret them. */

/* Linker Typedefs {{{ */
#ifndef DIABLOOBJECT_LINKERS_TYPEDEFS
#define DIABLOOBJECT_LINKERS_TYPEDEFS
typedef enum
{ NO_LINKER = 0, ADS_LINKER, GNU_LINKER } t_linker;
typedef enum
{
  TYPE_BOOL,           /*  0 */
  TYPE_VOID,           /*  1 */
  TYPE_OPER,           /*  2 */
  TYPE_STRING,         /*  3 */
  TYPE_NUMERIC,        /*  4 */
  TYPE_OTHER,          /*  5 */
  TYPE_ADDRESS,        /*  6 */
  TYPE_SECTION,        /*  7 */
  TYPE_SYMBOL,         /*  8 */
  TYPE_FUNCTION,       /*  9 */
  TYPE_TRIGGER         /* 10 */
} t_ast_types;

typedef enum
{
  OPER_AND,             /*  0 */
  OPER_OR,              /*  1 */
  OPER_ADD,             /*  2 */
  OPER_SUB,             /*  3 */
  OPER_DIV,             /*  4 */
  OPER_BITWISE_AND,     /*  5 */
  OPER_SHIFT_LEFT,      /*  6 */
  OPER_SHIFT_RIGHT,     /*  7 */
  OPER_COMMA,           /*  8 */
  OPER_EQUAL,           /*  9 */
  OPER_NOT_EQUAL,       /* 10 */
  OPER_NOT              /* 11 */
} t_ast_operators;

/* typedef for struct _t_ast_node */
typedef struct _t_ast_node t_ast_node;
typedef struct _t_ast_successors t_ast_successors;
typedef struct _t_linker_rule t_linker_rule;
typedef struct _t_ast_node_table_entry t_ast_node_table_entry;
#endif /* }}} Linkers Typedefs */
#include <diablosupport.h>
#ifdef DIABLOOBJECT_TYPES
/* Linker Types {{{ */
#ifndef DIABLOOBJECT_LINKERS_TYPES
#define DIABLOOBJECT_LINKERS_TYPES
/*! Struct to hold the successors of an ast node */
struct _t_ast_successors
{
  t_uint32 nargs;
  t_ast_node **args;
};

/*! Describes a linker action */
struct _t_ast_node
{
  t_ast_types type;
  /*! Union to hold the different types of data a linker action can have */
  union
  {
    t_string name;
    t_ast_operators oper;
    void *data;
    t_address addr;
  } data;
  t_ast_successors *args;
};

/*! A linker rule as passed from the linker script parser to the linker script
 * handler*/

struct _t_linker_rule
{
  t_ast_node *action;
  t_ast_node *trigger;
  t_ast_node *section;
  t_ast_node *address;
  t_ast_node *symbol;
};

/*! An entry in the linker function table */
struct _t_ast_node_table_entry
{
  t_string name;
  t_uint32 nargs;
  t_string arguments;
  t_string ret;
  void *(*fun) (t_ast_successors *, void *); /* TYPE */
};
#endif /* }}} Linker Types */
#endif
#ifdef DIABLOOBJECT_FUNCTIONS
/* Linker Functions {{{ */
#ifndef DIABLOOBJECT_LINKERS_FUNCTIONS
#define DIABLOOBJECT_LINKERS_FUNCTIONS
/* Ast nodes */
t_ast_successors *AstSuccessorsNew ();
t_ast_node *AstNodeFunctionNew (t_string, t_ast_successors *);
void AstSuccessorsAddSuccessor (t_ast_successors *, t_string, t_ast_types, t_ast_successors *);
t_ast_node *AstNodeOperatorNew (t_ast_operators, t_ast_node *, t_ast_node *);
void AstNodeFree (const t_ast_node *);

/* Linker rules */
t_linker_rule *LinkerRuleNew ();
void LinkerRuleExecute (t_linker_rule *, t_string);
void LinkerScriptParse(int mode, char * name_or_string, t_object * obj, t_symbol * sym);
void LinkerScriptInstallCallback(t_string name, t_uint32 nargs, t_string args, t_string returntype, void *(*fun)(t_ast_successors *, void *));
void LinkerScriptUninstallCallbacks();
#endif /* }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
