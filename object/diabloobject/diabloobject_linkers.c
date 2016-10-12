/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* This file is used to handle linker descriptions. A linker description is a
 * script file that describes how a specific linker for an architecture works:
 * what extra (so called linker defined) symbols it creates, what sections are
 * added by the linker, and what values these symbols and sections contain. A
 * linker description should eventually contain all architecture/linker specific
 * linking stuff.
 *
 * A linker descriptions consist to a large extend of a set of rules. These
 * rules describe a trigger (what condition needs to be met to execute the rule)
 * and an action (what does the rule do). All other parts of the linker rule are
 * data for the actions.
 *
 * Diablo parses the linker description (parser and lexer in
 * diablo_linker_script_parser.[ly]), and calls LinkerRuleExecute (in this file)
 * on each rule.
 *
 * LinkerRuleExecute finds out which rules of the script need to be executed by
 * evaluating the trigger and if necessary executes the rules. Triggers can
 * return four values (0, 1, 2, 3):
 *
 * - 0 means the rule is not to be executed. 
 * - 1 means we can execute the rule once  
 * - 2 means we do not need to execute the rule, but we need to re-execute it,
 *   because it might become true for other matching symbols. 
 * - 3 means execute and re-execute
 *
 * The last two cases occur when a trigger can be met several times.
 *
 * TODO(documentation) Describe different triggers and actions TODO(parser)
 * Handle functions as parameters TODO(FillerRelocated32) MATCHED_NAME should be
 * a callback function TODO(triggers) We need reloc matching triggers
 *
 * */

/* Includes {{{ */

#include <diabloobject.h>
#include <string.h>
#include <ctype.h>
/*}}}*/

/* LinkerScript specific IO: LINKER_SCRIPT_FATAL {{{ */
t_string current_rule_name = NULL;
void
LinkerIoWrapper (t_string message, ...)
{
  va_list ap;
  t_string linkermessage=StringIo("In linker rule %s: %s", current_rule_name, message);

  va_start (ap, message);
  InternalIo (io_wrapper_type, linkermessage, &ap);

  Free(linkermessage);
}

#define LINKER_SCRIPT_FATAL(mesg, ...) do { io_wrapper_file=__FILE__; io_wrapper_lnno=__LINE__; io_wrapper_type=E_FATAL; IoWrapper(mesg, ##__VA_ARGS__);\
  assert(false);\
  exit(-1); } while(0)/* }}} */
/* Global variables {{{ */
t_object *parser_object = NULL;
t_bool current_free_mode = FALSE;
t_address end_of_last_added;
t_string last_matched_symbol_name = NULL;
t_symbol *last_matched_symbol = NULL;
t_bool symbol_is_local = FALSE;

t_object *last_matched_object = NULL;
t_object *next_matched_object = NULL;
t_object *next_matched_object2 = NULL;
t_section *last_matched_section = NULL;
t_section *next_matched_section = NULL;

t_uint32 symbol_order = 10;
t_uint32 symbol_flags = 0;
t_tristate symbol_dup = FALSE;
t_uint32 next_matched_section_tel = 0;

/* }}} */
/* Static function/datastructure prototypes {{{ */
extern t_ast_node_table_entry FunctionTable[];
extern t_ast_node_table_entry SymbolFunctionTable[];
extern t_ast_node_table_entry AddressTable[];
extern t_ast_node_table_entry *InstalledCallbacksTable;

#define AstExecute(x,y,z,w) AstExecuteREAL(__FILE__,__LINE__,x,y,z,w)

static t_ast_node *AstExecuteREAL(const char *, int, t_ast_node *,
                                  t_ast_node_table_entry *, void *, t_bool);
static void CheckAndCastOneArgument (t_string, int, t_ast_node **, char,
                                     t_ast_node_table_entry *, void *);
static void CheckAndCastArguments (t_ast_node *, t_uint32, t_string,
                                   t_ast_node_table_entry *, void *);
static void *Vectorize (t_ast_successors *, void *);
static void *SectionStringChop (t_ast_successors *, void *);

/* }}} */

/* Abstract syntax tree manipulation {{{ */
void
AstPrint (FILE * file, const t_ast_node * in)
{
  t_ast_successors *args = in->args;

  fprintf (file, "\"%p\" [label=\"", in);
  switch (in->type)
  {
    case TYPE_FUNCTION:
      fprintf (file, "FUNC: %s", in->data.name);
      break;

    case TYPE_OPER:
      if (isprint (in->data.oper))
        fprintf (file, "OPER: %c", in->data.oper);
      else
        fprintf (file, "OPER: \\x%x", (unsigned) in->data.oper);
      break;
    case TYPE_NUMERIC:
      fprintf (file, "NUMBER");
      break;
    case TYPE_ADDRESS:
      fprintf (file, "ADDRESS");
      break;
    case TYPE_OTHER:
      fprintf (file, "OTHER %s", in->data.name);
      break;
    default:
      fprintf (file, "END");

  }
  printf ("\"]\n");
  if (args)
  {
    t_uint32 tel;

    for (tel = 0; tel < args->nargs; tel++)
    {
      fprintf (file, "\"%p\" -> \"%p\"\n", in, args->args[tel]);
      AstPrint (file, args->args[tel]);
    }
  }
}

t_ast_successors *
AstSuccessorsNew ()
{
  t_ast_successors *ret = Malloc (sizeof (t_ast_successors));

  ret->nargs = 0;
  ret->args = NULL;
  return ret;
}

void
AstSuccessorsAddSuccessor (t_ast_successors * args, t_string argument,
                           t_ast_types type, t_ast_successors * args2)
{
  args->nargs++;

  args->args = Realloc (args->args, args->nargs * sizeof (t_ast_node *));
  args->args[args->nargs - 1] = Malloc (sizeof (t_ast_node));
  args->args[args->nargs - 1]->type = type;
  args->args[args->nargs - 1]->data.data = argument;
  args->args[args->nargs - 1]->args = args2;
}

t_ast_node *
AstNodeFunctionNew (t_string name, t_ast_successors * args)
{
  t_ast_node *ret = Malloc (sizeof (t_ast_node));

  ret->type = TYPE_FUNCTION;
  ret->data.name = name;
  ret->args = args;
  return ret;
}

t_ast_node *
AstNodeOperatorNew (t_ast_operators oper, t_ast_node * left,
                    t_ast_node * right)
{
  t_ast_node *ret = Malloc (sizeof (t_ast_node));

  ret->type = TYPE_OPER;
  ret->data.oper = oper;

  ret->args = AstSuccessorsNew ();
  AstSuccessorsAddSuccessor (ret->args, left->data.data, left->type,
                             left->args);
  if (right != NULL)
  {
    AstSuccessorsAddSuccessor (ret->args, right->data.data, right->type,
                               right->args);
    Free (right);
  }
  Free (left);
  return ret;
}

void
AstNodeFree (const t_ast_node * in)
{
  t_ast_successors *args = in->args;
  t_string name = in->data.name;
  t_uint32 tel;

  if (args)
  {
    /* The number of arguments is right, now check and cast the arguments */

    for (tel = 0; tel < args->nargs; tel++)
    {
      if (args->args[tel])
      {
        switch (args->args[tel]->type)
        {
          case TYPE_STRING:
          case TYPE_OTHER:
            if (args->args[tel]->data.data) /* NULL Strings! */
              Free (args->args[tel]->data.data);
            break;
          default:
            break;
        }
        AstNodeFree (args->args[tel]);
      }
    }
    if (in->args->args)
      Free (in->args->args);
    Free (in->args);
  }

  if (in->type == TYPE_FUNCTION)
  {
    Free (name);
  }

  Free (in);
}

t_ast_node *
AstNodeDupInternal (const t_ast_node * in, t_ast_node * p)
{
  t_ast_node *ret = p;

  if (!in)
    return NULL;
  if (!ret)
    ret = Malloc (sizeof (t_ast_node));

  /* type and data {{{ */
  ret->type = in->type;

  switch (in->type)
  {
    case TYPE_OTHER:
    case TYPE_STRING:
    case TYPE_FUNCTION:
      ret->data.data = StringDup (in->data.data);
      break;
    default:
      ret->data.data = in->data.data;
      break;
  }
  /* }}} */
  /* arguments (recursive) {{{ */
  if (in->args)
  {
    t_uint32 tel;

    ret->args = Malloc (sizeof (t_ast_successors));
    ret->args->nargs = in->args->nargs;
    if (in->args->nargs)
      ret->args->args = Malloc (in->args->nargs * sizeof (t_ast_node *));
    else
      ret->args->args = NULL;
    for (tel = 0; tel < in->args->nargs; tel++)
    {
      ret->args->args[tel] =
        AstNodeDupInternal (in->args->args[tel], NULL);
    }
  }
  else
  {
    ret->args = NULL;
  }
  /* }}} */
  return ret;
}

t_ast_node *
AstNodeDup (const t_ast_node * in)
{
  return AstNodeDupInternal (in, NULL);
}

/*}}}*/
/* Linker rule manipulation {{{ */
t_linker_rule *
LinkerRuleNew ()
{
  t_linker_rule *ret = Malloc (sizeof (t_linker_rule));

  ret->action = ret->trigger = ret->section = ret->address = ret->symbol =
    NULL;
  return ret;
}

t_linker_rule *
LinkerRuleDup (const t_linker_rule * in)
{
  t_linker_rule *ret = Malloc (sizeof (t_linker_rule));

  ret->action = AstNodeDup (in->action);
  ret->trigger = AstNodeDup (in->trigger);
  ret->section = AstNodeDup (in->section);
  ret->address = AstNodeDup (in->address);
  ret->symbol = AstNodeDup (in->symbol);
  return ret;
}

/* }}} */

/* Callbacks for AST Execution {{{ */
/* The following functions are used during the execution of the AST's built by
 * the combination of the above functions, the lexer and the parser. They
 * implement the primitive functions for the Linker Description Language (LDL).
 * They are summarized in a table (FunctionTable), along with some type
 * information. */

/* FILLER FUNCTIONS {{{ */

/* TODO: Fix for 64 bit */
void *
FillerAddress (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_uint32 i = AddressExtractUint32(args->args[0]->data.addr);

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerAddress");
  SECTION_SET_DATA(sec,
                   Realloc (SECTION_DATA(sec),
                            AddressExtractUint32 (SECTION_CSIZE(sec)) + sizeof (t_uint32)));
  
  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    i = Uint32SwapEndian(i);

  memcpy (((char *) SECTION_DATA(sec)) + AddressExtractUint32 (SECTION_CSIZE(sec)),
          &i, sizeof (t_uint32));
  SECTION_SET_CSIZE(sec,
                    AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint32)));
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

void *
FillerConst64 (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_uint32 i = (t_uint32) (unsigned long) args->args[0]->data.data;

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerConst64");
  SECTION_SET_DATA(sec,
                   Realloc (SECTION_DATA(sec),
                            AddressExtractUint32 (SECTION_CSIZE(sec)) + sizeof (t_uint64)));
  
  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    i = Uint64SwapEndian(i);

  memcpy (((char *) SECTION_DATA(sec)) + AddressExtractUint32 (SECTION_CSIZE(sec)),
          &i, sizeof (t_uint64));
  SECTION_SET_CSIZE(sec,
                    AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint64)));
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

void *
FillerConst32 (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_uint32 i = (t_uint32) (unsigned long) args->args[0]->data.data;

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerConst32");
  SECTION_SET_DATA(sec,
                   Realloc (SECTION_DATA(sec),
                            AddressExtractUint32 (SECTION_CSIZE(sec)) + sizeof (t_uint32)));
  
  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    i = Uint32SwapEndian(i);

  memcpy (((char *) SECTION_DATA(sec)) + AddressExtractUint32 (SECTION_CSIZE(sec)),
          &i, sizeof (t_uint32));
  SECTION_SET_CSIZE(sec,
                    AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint32)));
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

void *
FillerConst16 (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_uint16 i = (t_uint16) (unsigned long) args->args[0]->data.data;

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerConst16");
  SECTION_SET_DATA(sec,
                   Realloc (SECTION_DATA(sec),
                            AddressExtractUint32 (SECTION_CSIZE(sec)) + sizeof (t_uint16)));
  
  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    i = Uint16SwapEndian(i);

  memcpy (((char *) SECTION_DATA(sec)) + AddressExtractUint32 (SECTION_CSIZE(sec)),
          &i, sizeof (t_uint16));
  SECTION_SET_CSIZE(sec,
                    AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint16)));
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

void *
FillerConst8 (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_uint8 i = (t_uint8) (unsigned long) args->args[0]->data.data;

  if (sec == NULL)
    FATAL(("Section should be set when calling FillerConst8"));
  SECTION_SET_DATA(sec,
                   Realloc (SECTION_DATA(sec),
                            AddressExtractUint32 (SECTION_CSIZE(sec)) + sizeof (t_uint8)));
  memcpy (((char *) SECTION_DATA(sec)) + AddressExtractUint32 (SECTION_CSIZE(sec)),
          &i, sizeof (t_uint8));
  SECTION_SET_CSIZE(sec,
                    AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint8)));
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

void *
FillerString (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_string i = (t_string) args->args[0]->data.data;

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerConst8");
  SECTION_SET_DATA(sec,
                   Realloc (SECTION_DATA(sec),
                            AddressExtractUint32 (SECTION_CSIZE(sec)) + strlen(i) ));
  memcpy (((char *) SECTION_DATA(sec)) + AddressExtractUint32 (SECTION_CSIZE(sec)),
          i, strlen(i));
  SECTION_SET_CSIZE(sec,
                    AddressAddUint32 (SECTION_CSIZE(sec), strlen(i)));
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

void *
FillerRelocated0 (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_string to_symbol = (t_string) args->args[1]->data.data;
  t_string assoc_symbol = (t_string) args->args[3]->data.data;
  t_string code = (t_string) args->args[5]->data.data;
  t_symbol *sym = NULL;
  t_symbol *assoc = NULL;

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerRelocated0");


  if (to_symbol)
    sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ObjectGetFromCache ("Linker", parser_object)),
                                     to_symbol, "R00*s0000", 0, TRUE, TRUE, T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), 
                                     AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);
  if (assoc_symbol)
    assoc = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ObjectGetFromCache ("Linker", parser_object)),
                                     assoc_symbol, "R00*s0000", 0, TRUE, TRUE, T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), 
                                     AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);

  RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)),
                              AddressNullForSection (sec),
                              T_RELOCATABLE(sec), AddressNullForObject(SECTION_OBJECT(sec)), sym,
                              TRUE, NULL, NULL, assoc, code);
  return NULL;
}

/* RELOCATED32 ( <int32:base_value>, <string:to_symbol>, <ignored>, <string:secondary_symbol>, <ignored>, <code>) */

void *
FillerRelocated32 (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_uint32 i = (t_uint32) (unsigned long) args->args[0]->data.data;
  t_string to_symbol = (t_string) args->args[1]->data.data;
  t_int32 addend = (t_int32) (unsigned long) args->args[2]->data.data;
  t_string assoc_symbol = (t_string) args->args[3]->data.data;
  t_int32 assoc_addend = (t_int32) (unsigned long) args->args[4]->data.data;
  t_string code = (t_string) args->args[5]->data.data;
  t_symbol *sym = NULL;
  t_symbol *assoc = NULL;
  t_reloc * rel = NULL;

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerRelocated32");

  SECTION_SET_DATA(sec, Realloc (SECTION_DATA(sec), AddressExtractUint32 (SECTION_CSIZE(sec)) + sizeof (t_uint32)));

  if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
    i = Uint32SwapEndian(i);

  memcpy (((char *) SECTION_DATA(sec)) + AddressExtractUint32 (SECTION_CSIZE(sec)), &i, sizeof (t_uint32));


  if (to_symbol && (strcmp (to_symbol, "MATCHED_NAME") == 0))
	  LINKER_SCRIPT_FATAL("Deprecated use of MATCHED_NAME. Use MATCHED_NAME() instead");
  else if (to_symbol)
    sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ObjectGetFromCache ("Linker", parser_object)),
                                     to_symbol, "R00*s0000", 0, TRUE, TRUE, T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), 
                                     AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);
  if (assoc_symbol)
    assoc = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ObjectGetFromCache ("Linker", parser_object)),
                                     assoc_symbol, "R00*s0000", 0, TRUE, TRUE, T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), 
                                     AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);

  rel = RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)),
                              AddressNewForSection (sec, (t_uint32) addend),
                              T_RELOCATABLE(sec), SECTION_CSIZE(sec), sym,
                              TRUE, NULL, NULL, assoc, code);

  /* also add assoc_addend to relocation */
  if (StringPatternMatch("*A01*", code))
  {
    RELOC_SET_N_ADDENDS(rel, 2);
    RELOC_SET_ADDENDS(rel, Realloc(RELOC_ADDENDS(rel), 2*sizeof(t_address)));
    RELOC_ADDENDS(rel)[1] = AddressNewForSection (sec, (t_uint32)assoc_addend);
  }

  SECTION_SET_CSIZE(sec,
                    AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint32)));
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

void *
FillerRelocated64 (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_uint32 i = (t_uint32) (unsigned long) args->args[0]->data.data;
  t_string to_symbol = (t_string) args->args[1]->data.data;
  t_int32 addend = (t_int32) (unsigned long) args->args[2]->data.data;
  t_string assoc_symbol = (t_string) args->args[3]->data.data;
  t_int32 assoc_addend = (t_int32) (unsigned long) args->args[4]->data.data;
  t_string code = (t_string) args->args[5]->data.data;
  t_symbol *sym = NULL;
  t_symbol *assoc = NULL;
  t_reloc * rel = NULL;

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerRelocated64");
  SECTION_SET_DATA(sec,
                   Realloc (SECTION_DATA(sec),
                            AddressExtractUint32 (SECTION_CSIZE(sec)) + sizeof (t_uint64)));
  memcpy (((char *) SECTION_DATA(sec)) + AddressExtractUint32 (SECTION_CSIZE(sec)),
          &i, sizeof (t_uint32));
  if (to_symbol && (strcmp (to_symbol, "MATCHED_NAME") == 0))
    LINKER_SCRIPT_FATAL("Use of deprecated MATCHED_NAME. Use MATCHED_NAME() instead!");
  else if (to_symbol)
    sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ObjectGetFromCache ("Linker", parser_object)),
                                     to_symbol, "R00*s0000", 0, TRUE, TRUE, T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), 
                                     AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);
  if (assoc_symbol)
    assoc = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ObjectGetFromCache ("Linker", parser_object)),
                                     assoc_symbol, "R00*s0000", 0, TRUE, TRUE, T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), 
                                     AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);

  rel = RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)),
                              AddressNewForSection (sec, (t_uint64)(t_int64)addend),
                              T_RELOCATABLE(sec), SECTION_CSIZE(sec), sym,
                              TRUE, NULL, NULL, assoc, code);

  /* also add assoc_addend to relocation */
  if (StringPatternMatch("*A01*", code))
  {
    RELOC_SET_N_ADDENDS(rel, 2);
    RELOC_SET_ADDENDS(rel, Realloc(RELOC_ADDENDS(rel), 2*sizeof(t_address)));
    RELOC_ADDENDS(rel)[1] = AddressNewForSection (sec, (t_uint64)(t_int64)assoc_addend);
  }

  SECTION_SET_CSIZE(sec,
                    AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint64)));
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

void *
FillerSectionOffset (t_ast_successors * args, void *tsec)
{
  t_section * sec = tsec;
  t_section * to_sec = (t_section *) args->args[0]->data.data;
  t_address addr = args->args[1]->data.addr;

  if (OBJECT_ADDRESS_SIZE(parser_object) == ADDRSIZE32)
  {
    t_uint32 i = 0;
    SECTION_SET_DATA(sec, Realloc (SECTION_DATA(sec), AddressExtractUint32 (SECTION_CSIZE(sec)) + sizeof (t_uint32)));
    memcpy (((char *) SECTION_DATA(sec)) + AddressExtractUint32 (SECTION_CSIZE(sec)), &i, sizeof (t_uint32));
    RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)), AddressNullForSection (sec), T_RELOCATABLE(sec), SECTION_CSIZE(sec), T_RELOCATABLE(to_sec), AddressSub(addr, SECTION_CADDRESS(to_sec)) , TRUE, NULL, NULL, NULL, "R00\\l*w\\s0000$");
  }
  else
  {
    t_uint64 i = 0;
    SECTION_SET_DATA(sec, Realloc (SECTION_DATA(sec), AddressExtractUint32 (SECTION_CSIZE(sec)) + sizeof (t_uint64)));
    memcpy (((char *) SECTION_DATA(sec)) + AddressExtractUint32 (SECTION_CSIZE(sec)), &i, sizeof (t_uint64));
    RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)), AddressNullForSection (sec), T_RELOCATABLE(sec), SECTION_CSIZE(sec), T_RELOCATABLE(to_sec), AddressSub(addr, SECTION_CADDRESS(to_sec)) , TRUE, NULL, NULL, NULL, "R00\\L*W\\s0000$");
  }



  if (OBJECT_ADDRESS_SIZE(parser_object) == ADDRSIZE32)
    SECTION_SET_CSIZE(sec, AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint32)));
  else
    SECTION_SET_CSIZE(sec, AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint64)));
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

void *
FillerStartOfSection (t_ast_successors * args, void *tsec)
{
  t_string tmp;
  t_section *sec = tsec;
  t_string s = (t_string) args->args[0]->data.data;
  t_section *sec2 = SectionGetFromObjectByName (parser_object, s);

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerStartOfSection");
  if (!sec2)
    LINKER_SCRIPT_FATAL("In Filler Start Of Section: section not found!");

  if (OBJECT_ADDRESS_SIZE(parser_object) == ADDRSIZE32)
  {
    t_uint32 i = 0;
    t_symbol *sym;
    tmp = StringConcat3 ("$$Linker", "_", s);
    sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ObjectGetFromCache ("Linker", parser_object)), tmp, "R00*s0000", 0, TRUE, TRUE,
      T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);
    Free (tmp);

    SECTION_SET_DATA(sec,
                     Realloc (SECTION_DATA(sec),
                              AddressExtractUint32 (SECTION_CSIZE(sec)) +
                              sizeof (t_uint32)));
    memcpy (((char *) SECTION_DATA(sec)) +
            AddressExtractUint32 (SECTION_CSIZE(sec)), &i,
            sizeof (t_uint32));

    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)),
                                AddressNullForSection (sec),
                                T_RELOCATABLE(sec), SECTION_CSIZE(sec),
                                sym, FALSE, NULL, NULL, NULL,
                                "S00\\" WRITE_32);
    SECTION_SET_CSIZE(sec,
                      AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint32)));
  }
  else
  {
    t_uint64 i = 0;
    t_symbol *sym;
    tmp = StringConcat3 ("$$Linker", "_", s);
    sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ObjectGetFromCache ("Linker", parser_object)), tmp, "R00*s0000", 0, TRUE, TRUE,
      T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);
    Free (tmp);

    SECTION_SET_DATA(sec,
                     Realloc (SECTION_DATA(sec),
                              AddressExtractUint32 (SECTION_CSIZE(sec)) +
                              sizeof (t_uint64)));
    memcpy (((char *) SECTION_DATA(sec)) +
            AddressExtractUint32 (SECTION_CSIZE(sec)), &i,
            sizeof (t_uint64));

    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)),
                                AddressNullForSection (sec),
                                T_RELOCATABLE(sec), SECTION_CSIZE(sec),
                                sym, FALSE, NULL, NULL, NULL,
                                "S00\\" WRITE_64);
    SECTION_SET_CSIZE(sec,
                      AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint64)));
  }

  tmp = StringConcat3 ("$$Linker", "_", s);
  SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), tmp, "R00A00+$", 10, PERHAPS, FALSE, 
    T_RELOCATABLE(sec2), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);
  Free (tmp);

  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));


  return NULL;
}

void *
FillerEndOfSection (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_string s = (t_string) args->args[0]->data.data;
  t_section *sec2 = SectionGetFromObjectByName (parser_object, s);
  t_string tmp = StringConcat3 ("$$Linker", "_endof_", s);
  t_symbol *sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ObjectGetFromCache ("Linker", parser_object)), tmp, "R00*s0000", 0, TRUE, TRUE,
    T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);
  Free(tmp);

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerEndOfSection");
  if (!sec2)
    LINKER_SCRIPT_FATAL("In Filler Size Of Section: section not found!");

  if (OBJECT_ADDRESS_SIZE(parser_object) == ADDRSIZE32)
  {
    t_uint32 i = 0;

    SECTION_SET_DATA(sec,
                     Realloc (SECTION_DATA(sec),
                              AddressExtractUint32 (SECTION_CSIZE(sec)) +
                              sizeof (t_uint32)));
    memcpy (((char *) SECTION_DATA(sec)) +
            AddressExtractUint32 (SECTION_CSIZE(sec)), &i,
            sizeof (t_uint32));
    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)),
                                AddressNullForSection (sec),
                                T_RELOCATABLE(sec), SECTION_CSIZE(sec),
                                sym, FALSE, NULL, NULL, NULL,
                                "S00Z+\\" WRITE_32);
    SECTION_SET_CSIZE(sec,
                      AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint32)));
  }
  else
  {
    t_uint64 i = 0;

    if (sec == NULL)
      LINKER_SCRIPT_FATAL("Section should be set when calling FillerEndOfSection");
    SECTION_SET_DATA(sec,
                     Realloc (SECTION_DATA(sec),
                              AddressExtractUint32 (SECTION_CSIZE(sec)) +
                              sizeof (t_uint64)));
    memcpy (((char *) SECTION_DATA(sec)) +
            AddressExtractUint32 (SECTION_CSIZE(sec)), &i,
            sizeof (t_uint64));
    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)),
                                AddressNullForSection (sec),
                                T_RELOCATABLE(sec), SECTION_CSIZE(sec),
                                sym, FALSE, NULL, NULL, NULL,
                                "S00Z+\\" WRITE_64);
    SECTION_SET_CSIZE(sec,
                      AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint64)));
  }
  
  tmp = StringConcat3 ("$$Linker", "_endof_", s);
  SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), tmp, "R00A00+", 0, PERHAPS, FALSE, T_RELOCATABLE(sec2),
    AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);
  Free (tmp);
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

void *
FillerSizeOfSection (t_ast_successors * args, void *tsec)
{
  t_section *sec = tsec;
  t_string s = (t_string) args->args[0]->data.data;
  t_section *sec2 = SectionGetFromObjectByName (parser_object, s);
  t_string tmp = StringConcat3 ("$$Linker", "_sizeof_", s);
  t_symbol *sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ObjectGetFromCache ("Linker", parser_object)), tmp, "Z00$", 0, TRUE, TRUE,
    T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);
  Free (tmp);

  if (sec == NULL)
    LINKER_SCRIPT_FATAL("Section should be set when calling FillerSizeOfSection");
  if (!sec2)
    LINKER_SCRIPT_FATAL("In Filler Size Of Section: section not found!");

  if (OBJECT_ADDRESS_SIZE(parser_object) == ADDRSIZE32)
  {
    t_uint32 i = 0;

    if (sec == NULL)
      LINKER_SCRIPT_FATAL("Section should be set when calling FillerStartOfSection");
    SECTION_SET_DATA(sec,
                     Realloc (SECTION_DATA(sec),
                              AddressExtractUint32 (SECTION_CSIZE(sec)) +
                              sizeof (t_uint32)));
    memcpy (((char *) SECTION_DATA(sec)) +
            AddressExtractUint32 (SECTION_CSIZE(sec)), &i,
            sizeof (t_uint32));
    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)),
                                AddressNullForSection (sec),
                                T_RELOCATABLE(sec), SECTION_CSIZE(sec),
                                sym, FALSE, NULL, NULL, NULL,
                                "S00\\" WRITE_32);
    SECTION_SET_CSIZE(sec,
                      AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint32)));
  }
  else
  {
    t_uint64 i = 0;

    if (sec == NULL)
      LINKER_SCRIPT_FATAL("Section should be set when calling FillerStartOfSection");
    SECTION_SET_DATA(sec,
                     Realloc (SECTION_DATA(sec),
                              AddressExtractUint32 (SECTION_CSIZE(sec)) +
                              sizeof (t_uint64)));
    memcpy (((char *) SECTION_DATA(sec)) +
            AddressExtractUint32 (SECTION_CSIZE(sec)), &i,
            sizeof (t_uint64));
    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE(SECTION_OBJECT(sec)),
                                AddressNullForSection (sec),
                                T_RELOCATABLE(sec), SECTION_CSIZE(sec),
                                sym, FALSE, NULL, NULL, NULL,
                                "S00\\" WRITE_64);
    SECTION_SET_CSIZE(sec,
                      AddressAddUint32 (SECTION_CSIZE(sec), sizeof (t_uint64)));
  }

  tmp = StringConcat3 ("$$Linker", "_sizeof_", s);
  SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), tmp, "Z00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec2),
    AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), 0);
  Free (tmp);
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));
  return NULL;
}

/*END FILLER }}} */
/* TRIGGER FUNCTIONS {{{ */

void *
TriggerAlways (t_ast_successors * args, void *ignored)
{
  return (void *) TRUE;
}

t_symbol *trigger_pattern_symbol = NULL;

void *
TriggerLinkedSymbolExists (t_ast_successors * args, void *ignored)
{
  t_uint32 tel;
  t_bool pattern = FALSE;
  char *name = args->args[0]->data.data;

  for (tel = 0; tel < strlen (name); tel++)
  {
    if (name[tel] == '*')
    {
      pattern = TRUE;
      break;
    }
  }

  if (pattern)
  {
    if (!trigger_pattern_symbol)
    {
      VERBOSE(3,("LinkedSymbolExists: starting new sequence for pattern %s",name));
      trigger_pattern_symbol = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(parser_object));
    }

    for (; trigger_pattern_symbol != NULL; trigger_pattern_symbol = SYMBOL_NEXT(trigger_pattern_symbol))
    {
      if (!SYMBOL_NAME(trigger_pattern_symbol))
        continue;
      if ((StringPatternMatch (name, SYMBOL_NAME(trigger_pattern_symbol))))
      {
        if (last_matched_symbol_name) Free(last_matched_symbol_name);
        last_matched_symbol_name = StringDup (SYMBOL_NAME(trigger_pattern_symbol));
        last_matched_symbol = trigger_pattern_symbol;
        trigger_pattern_symbol = SYMBOL_NEXT(trigger_pattern_symbol);
        VERBOSE(3,("LinkedSymbolExists: returning for pattern %s: %s",name,last_matched_symbol_name));
        return (void *) (unsigned long) (trigger_pattern_symbol? 3 : 1);
      }
    }
    VERBOSE(3,("LinkedSymbolExists: pattern %s finished",name));
    return (void *) 0;
  }
  else
  {
    t_symbol * sym;
    sym = SymbolTableGetSymbolByName (OBJECT_SYMBOL_TABLE(parser_object), name);
    if (sym)
    {
    last_matched_symbol = sym;
    if (last_matched_symbol_name) Free(last_matched_symbol_name);
    last_matched_symbol_name = StringDup (SYMBOL_NAME(sym));
    VERBOSE(3,("LinkedSymbolExists: returning non-pattern %s",name,last_matched_symbol_name));
    sym = NULL;
    return (void *) 1; 
    }
    else
    {
    /* do not set last_matched_symbol etc to NULL, this can be
     * used in checks like "LINKED_SYMBOL_EXISTS(x) &&
     * !LINKED_SYMBOL_EXISTS(y)"
     */
    VERBOSE(3,("LinkedSymbolExists: not found %s",name));
    return (void *) 0; 
    }
  }

  return (void *) 0; /* Keep compiler happy */
}

void *
TriggerSymbolExists (t_ast_successors * args, void *ignored)
{
  static t_symbol *sym = NULL;
  t_uint32 tel;
  t_bool pattern = FALSE;
  char *name = args->args[0]->data.data;

  for (tel = 0; tel < strlen (name); tel++)
  {
    if (name[tel] == '*')
    {
      pattern = TRUE;
      break;
    }
  }

  if (pattern)
  {
    if (!sym)
      sym = SYMBOL_TABLE_FIRST(OBJECT_SUB_SYMBOL_TABLE(parser_object));

    for (; sym != NULL; sym = SYMBOL_NEXT(sym))
    {
      if (!SYMBOL_NAME(sym))
        continue;
      if ((StringPatternMatch (name, SYMBOL_NAME(sym))))
      {
        if (last_matched_symbol_name) Free(last_matched_symbol_name);
        last_matched_symbol_name = StringDup (SYMBOL_NAME(sym));
        last_matched_symbol = sym;
        sym = SYMBOL_NEXT(sym);
        if (!sym) 
          return (void *) 1;
        else
          return (void *) 3;
      }
    }
    return (void *) 0; 
  }
  else
  {
    sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(parser_object), name);
    if (sym)
    {
    /* last_matched_symbol = sym;
    if (last_matched_symbol_name) Free(last_matched_symbol_name);
    last_matched_symbol_name = StringDup (SYMBOL_NAME(sym)); */
    sym = NULL;
    return (void *) 1; 
    }
    else
    {
    /* last_matched_symbol =  NULL;
    if (last_matched_symbol_name) Free(last_matched_symbol_name);
    last_matched_symbol_name = NULL; */
    sym = NULL;
    return (void *) 0; 
    }
  }

  return (void *) 0; 
}

void *
TriggerUndefinedSymbol (t_ast_successors * args, void *ignored)
{
  t_symbol *msym;

  msym =
    SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(parser_object),
                                args->args[0]->data.data);
  VERBOSE(1, ("Checking for undefinedness of %s", args->args[0]->data.data));
  if (!msym)
  {
    VERBOSE(1, ("FALSE (not found)"));
    return (void *) FALSE;
  }

  if (SYMBOL_SEARCH(msym))
  {
    VERBOSE(1, ("TRUE"));
    return (void *) TRUE;
  }
  else
  {
    VERBOSE(1, ("@S FALSE (not undefined)", msym));
    return (void *) FALSE;
  }
}

void *
TriggerUndefOrNonExistSymbol(t_ast_successors * args, void *ignored)
{
  t_symbol *msym;

  msym =
    SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(parser_object),
                                args->args[0]->data.data);
  VERBOSE(1, ("Checking for undefinedness or non-existence of %s", args->args[0]->data.data));
  if (!msym)
  {
    VERBOSE(1, ("TRUE (not found)"));
    return (void *) TRUE;
  }

  if (SYMBOL_SEARCH(msym))
  {
    VERBOSE(1, ("TRUE"));
    return (void *) TRUE;
  }
  else
  {
    VERBOSE(1, ("@S FALSE (not undefined)", msym));
    return (void *) FALSE;
  }
}

void *
TriggerWeakUndefinedSymbol (t_ast_successors * args, void *ignored)
{
  t_symbol *msym;

  msym =
    SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(parser_object),
                                args->args[0]->data.data);
  VERBOSE(1, ("Checking for weak undefinedness of %s", args->args[0]->data.data));
  if (!msym)
  {
    VERBOSE(1, ("FALSE (not found)"));
    return (void *) FALSE;
  }

  if ((!(SYMBOL_SEARCH(msym)))&&(SYMBOL_BASE(msym)==(t_relocatable*)OBJECT_UNDEF_SECTION(parser_object)))
  {
    VERBOSE(1, ("TRUE"));
    return (void *) TRUE;
  }
  else
  {
    VERBOSE(1, ("@S FALSE (not undefined)", msym));
    return (void *) FALSE;
  }
}



void *
TriggerSubsectionExists (t_ast_successors * args, void *ignored)
{
  t_uint32 tel;
  t_object *obj, *tmp;
  t_section *sec;
  t_bool subobject_pattern = FALSE;
  t_bool subsection_pattern = FALSE;
  char *subobject_name = args->args[0]->data.data;
  char *subsection_name = args->args[1]->data.data;

  VERBOSE(1, ("Checking if subsection %s exists in object %s",
              subsection_name, subobject_name));

  for (tel = 0; tel < strlen (subobject_name); tel++)
  {
    if (subobject_name[tel] == '*')
    {
      subobject_pattern = TRUE;
      break;
    }
  }

  for (tel = 0; tel < strlen (subsection_name); tel++)
  {
    if (subsection_name[tel] == '*')
    {
      subsection_pattern = TRUE;
      break;
    }
  }

  if (subobject_pattern || subsection_pattern)
  {
    if (next_matched_object)
      tmp = next_matched_object;
    else
      tmp = OBJECT_MAPPED_FIRST(parser_object);
    next_matched_object = NULL;

    for (; tmp != NULL; tmp = OBJECT_NEXT(tmp))
    {
      if (next_matched_object2)
        obj = next_matched_object2;
      else
        obj = tmp;
      next_matched_object2 = NULL;

      for (; obj != NULL; obj = OBJECT_EQUAL(obj))
      {
        /*printf("Doing subsection exists %s\n",OBJECT_NAME(obj)); */
        if (StringPatternMatch (subobject_name, OBJECT_NAME(obj)))
        {
          tel = next_matched_section_tel;
          if (tel && !next_matched_section)
          {
            tel += 0x100000;
            tel &= ~0xfffff;
          }
          next_matched_section_tel = 0;
          for (; (tel & 0x1f00000) < 0x500000;
               tel += 0x100000, tel &= ~0xfffff)
          {

            if (next_matched_section)
            {
              sec = next_matched_section;
              next_matched_section = NULL;
            }
            else
            {
              sec =
                SECTIONARRAYBYNUMBER(obj,
                                     tel) ?
                SECTIONARRAYBYNUMBER(obj, tel)[0] : NULL;
            }

            for (;
                 ((tel & 0xfffff) <
                  SECTIONCOUNTBYNUMBER(obj, tel));
                 tel++, sec =
                 ((tel & 0xfffff) <
                  SECTIONCOUNTBYNUMBER(obj,
                                       tel)) ?
                 SECTIONARRAYBYNUMBER(obj,
                                      tel)[tel & 0xfffff] : NULL)
            {
              /* printf("Doing exists %s\n",SECTION_NAME(sec)); */
              if (StringPatternMatch (subsection_name, SECTION_NAME(sec)))
              {
                last_matched_object = obj;
                next_matched_object = tmp;
                next_matched_object2 = obj;
                last_matched_section = sec;
                next_matched_section_tel = tel + 1;
                next_matched_section =
                  ((next_matched_section_tel & 0xfffff) <
                   SECTIONCOUNTBYNUMBER(obj,
                                        next_matched_section_tel))
                  ? SECTIONARRAYBYNUMBER(obj,
                                         next_matched_section_tel)
                  [next_matched_section_tel & 0xfffff] : NULL;
                VERBOSE(1, ("Matched %s %s for pattern %s %s",
                            OBJECT_NAME(obj), SECTION_NAME(sec), subobject_name,
                            subsection_name));
                return (void *) 3;
              }
            }
          }
        }
      }
    }
    VERBOSE(1, ("FALSE"));
    return (void *) FALSE;

  }
  else
  {
    obj = ObjectGetFromCache (subobject_name, parser_object);
    if (!obj)
    {
      VERBOSE(1, ("FALSE"));
      return (void *) FALSE;
    }
    sec = SectionGetFromObjectByName (obj, subsection_name);
    if (!sec)
    {
      VERBOSE(1, ("FALSE"));
      return (void *) FALSE;
    }
    VERBOSE(1, ("TRUE"));
    return (void *) TRUE;
  }
}

void *
GetSubSectionContainingAddress (t_ast_successors * args, void *ignored)
{
  t_address  addr = args->args[0]->data.addr;
  t_object * tmp;

  for (tmp = OBJECT_MAPPED_FIRST(parser_object); tmp != NULL; tmp = OBJECT_NEXT(tmp))
  {
    t_object * obj;
    for (obj = tmp; obj != NULL; obj = OBJECT_EQUAL(obj))
    {

      t_section * sec = SectionGetFromObjectByAddress (obj, addr);

      if (sec)
      {
        return sec;
      }
    }
  }
  LINKER_SCRIPT_FATAL("Section containing address not found");
}

void *
TriggerSectionExists (t_ast_successors * args, void *ignored)
{
  t_section *sec;

  sec = SectionGetFromObjectByName (parser_object, args->args[0]->data.data);
  VERBOSE(1, ("Checking if section %s exists", args->args[0]->data.data));
  if (!sec)
  {
    VERBOSE(1, ("FALSE"));
    return (void *) FALSE;
  }
    VERBOSE(1, ("TRUE"));
  return (void *) TRUE;
}


void *
TriggerGeneratePIC(t_ast_successors * args, void *ignored)
{
  switch (OBJECT_TYPE(parser_object))
  {
    case OBJTYP_EXECUTABLE:
    case OBJTYP_SHARED_LIBRARY:
      return (void*)0;
    case OBJTYP_SHARED_LIBRARY_PIC:
    case OBJTYP_EXECUTABLE_PIC:
      return (void*)1;
    default:
      FATAL(("Unhandled objec type: %d",OBJECT_TYPE(parser_object)));
      break;
  }
  FATAL(("cannot get here"));
  return NULL;
}

/* END TRIGGER }}} */
/* ACTION FUNCTIONS {{{ */

static int
__helper_cmp_syms (const void *a, const void *b)
{
  t_symbol *A = *(t_symbol **) a;
  t_symbol *B = *(t_symbol **) b;

  if (AddressIsGt (SYMBOL_OFFSET_FROM_START(B), SYMBOL_OFFSET_FROM_START(A)))
    return 1;
  return -1;
}

void *
SectionStringChop (t_ast_successors * args, void *rule)
{
  t_object *obj;
  t_section *sec;
  int tel = 0;
  int nchops = 0;
  int len;
  int total_len = 0;
  int first_len = 0;
  t_symbol *continue_here;

  obj = ObjectGetFromCache (args->args[0]->data.data, parser_object);
  if (!obj)
    LINKER_SCRIPT_FATAL("Could not find object %s to chop %s\n", args->args[0]->data.data, args->args[1]->data.data);
  sec = SectionGetFromObjectByName (obj, args->args[1]->data.data);
  if (!sec)
    LINKER_SCRIPT_FATAL("Could not find section %s in object %s to chop\n", args->args[1]->data.data, args->args[0]->data.data);

  if (SECTION_REFERS_TO(sec))
    LINKER_SCRIPT_FATAL("String section %s:%s contains relocations!", args->args[0]->data.data, args->args[1]->data.data);

  /* in order to speed up moving the symbols to the newly
   * created sections, we first sort them by address */
  {
    t_symbol **symarr;
    t_symbol *iter;
    t_uint32 i, nsyms = 0;

    for (iter = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(obj));
         iter;
         iter = SYMBOL_NEXT(iter))
      nsyms++;
    symarr = Malloc (nsyms * sizeof (t_symbol *));

    if (SYMBOL_TABLE_NSYMS(OBJECT_SYMBOL_TABLE(obj)) != nsyms)
      LINKER_SCRIPT_FATAL("Symbol table corrupt! object %s %u <> %u", OBJECT_NAME (obj), nsyms, SYMBOL_TABLE_NSYMS(OBJECT_SYMBOL_TABLE(obj)));

    for (i = 0, iter = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(obj));
         iter;
         iter = SYMBOL_NEXT(iter))
      symarr[i++] = iter;
    diablo_stable_sort (symarr, i, sizeof (t_symbol *), __helper_cmp_syms);
    for (i = 1; i < nsyms; i++)
    {
      SYMBOL_SET_PREV(symarr[i], symarr[i - 1]);
      SYMBOL_SET_NEXT(symarr[i - 1], symarr[i]);
    }
    SYMBOL_SET_PREV(symarr[0], NULL);
    SYMBOL_SET_NEXT(symarr[nsyms - 1], NULL);
    SYMBOL_TABLE_SET_FIRST(OBJECT_SYMBOL_TABLE(obj), symarr[0]);
    SYMBOL_TABLE_SET_LAST(OBJECT_SYMBOL_TABLE(obj), symarr[nsyms - 1]);
    Free (symarr);

    continue_here = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(obj));
  }

  while (tel < AddressExtractUint32 (SECTION_CSIZE(sec)))
  {
    nchops++;
    len = 0;
    for (; tel < AddressExtractUint32 (SECTION_CSIZE(sec)); tel++)
    {
      len++;
      total_len++;
      if (((char *) SECTION_DATA(sec))[tel] == 0)
        break;
    }
    if (nchops != 1)
    {
      char buffer[80];
      t_section *string_sec;
      t_reloc_ref *iter;
      t_symbol *sym;

      /* create new section to store the string */
      sprintf (buffer, ".stringified.%03d%s", nchops, SECTION_NAME(sec));
      string_sec = SectionCreateForObject (
                                           obj, SECTION_TYPE(sec), SECTION_PARENT_SECTION(sec),
                                           AddressNewForSection (sec, len), buffer);
      VERBOSE(1, ("Creating new section %s", SECTION_NAME(string_sec)));
      strcpy (SECTION_DATA(string_sec), ((char *) SECTION_DATA(sec)) + total_len - len);
      SECTION_SET_OLD_ADDRESS(string_sec,
                              AddressAddUint32 (SECTION_OLD_ADDRESS(sec), total_len - len));
      SECTION_SET_CADDRESS(string_sec,
                           AddressAddUint32 (SECTION_CADDRESS(sec), total_len - len));
      /* Dominique: string sections should only be aligned at alignment 1 (not sure though) */
      SECTION_SET_ALIGNMENT(string_sec, AddressNewForSection (sec, 1));

      /* move all symbols to the new section */
      for (iter = SECTION_REFED_BY(sec); iter != NULL;
           iter = RELOC_REF_NEXT(iter))
      {
        t_uint32 i;
        for (i=0; i<RELOC_N_TO_SYMBOLS(RELOC_REF_RELOC(iter)); i++)
        {
          if ((SYMBOL_BASE(RELOC_TO_SYMBOL(RELOC_REF_RELOC(iter))[i])==T_RELOCATABLE(sec)) 
              && (AddressExtractUint32 (SYMBOL_OFFSET_FROM_START(RELOC_TO_SYMBOL(RELOC_REF_RELOC(iter))[i])) < total_len)
              && (AddressExtractUint32 (SYMBOL_OFFSET_FROM_START(RELOC_TO_SYMBOL(RELOC_REF_RELOC(iter))[i])) >= (total_len - len)))
          {
	    SymbolSetSymbolBaseAndUpdateRelocs (OBJECT_RELOC_TABLE(parser_object), RELOC_TO_SYMBOL(RELOC_REF_RELOC(iter))[i], T_RELOCATABLE(string_sec));

            SYMBOL_SET_OFFSET_FROM_START(RELOC_TO_SYMBOL(RELOC_REF_RELOC(iter))[i], AddressSub (SYMBOL_OFFSET_FROM_START(RELOC_TO_SYMBOL(RELOC_REF_RELOC(iter))[i]), AddressSub (SECTION_CADDRESS(string_sec), SECTION_CADDRESS(sec))));
            SymbolSetBase(SYMBOL_MAPPED(RELOC_TO_SYMBOL(RELOC_REF_RELOC(iter))[i]), T_RELOCATABLE(string_sec));
            SYMBOL_SET_OFFSET_FROM_START(SYMBOL_MAPPED(RELOC_TO_SYMBOL(RELOC_REF_RELOC(iter))[i]), SYMBOL_OFFSET_FROM_START(RELOC_TO_SYMBOL(RELOC_REF_RELOC(iter))[i]));
          }
        }
      }
      /* now do the symbols */
      /* symbols from the subobject */
      for (sym = continue_here; sym; sym = SYMBOL_NEXT(sym), continue_here = sym)
      {
        if (AddressExtractUint32 (SYMBOL_OFFSET_FROM_START(sym)) <
            (total_len - len))
          continue; /* we haven't yet reached the symbols of this section */

        if (AddressExtractUint32 (SYMBOL_OFFSET_FROM_START(sym)) >= total_len)
          break; /* we've reached the first symbol past the section */

        if (T_SECTION(SYMBOL_BASE(sym)) == sec)
        {
          SymbolSetBase(sym, T_RELOCATABLE(string_sec));
          SYMBOL_SET_OFFSET_FROM_START(sym, AddressSub (SYMBOL_OFFSET_FROM_START(sym), AddressSub (SECTION_CADDRESS(string_sec), SECTION_CADDRESS(sec))));
          SymbolSetBase(SYMBOL_MAPPED(sym), T_RELOCATABLE(string_sec));
          SYMBOL_SET_OFFSET_FROM_START(SYMBOL_MAPPED(sym), SYMBOL_OFFSET_FROM_START(sym));
        }
      }
    }
    else
    {
      first_len = len;
    }
    tel++;
  }

  SECTION_SET_CSIZE(sec, AddressNewForSection (sec, first_len));
  SECTION_SET_OLD_SIZE(sec, SECTION_CSIZE(sec));

  VERBOSE(1, ("Chopping in %d pieces\n", nchops));

  return NULL;
}

/* {{{ helper function for OverrideSectionType */
static void
MoveSection (
             t_section * sec, char sectype,
             size_t fromarr_offset, size_t fromcount_offset,
             size_t toarr_offset, size_t tocount_offset)
{
  t_object *obj = SECTION_OBJECT(sec);
  t_section **froms, **tos;
  t_section ***froms_p, ***tos_p;
  int i, j, nfroms, ntos;
  t_uint32 *nfroms_p, *ntos_p;

  /* get all the right pointers */
  froms_p = ((t_section ***) (((char *) obj) + fromarr_offset));
  nfroms_p = ((t_uint32 *) (((char *) obj) + fromcount_offset));
  froms = *froms_p;
  nfroms = *nfroms_p;
  tos_p = ((t_section ***) (((char *) obj) + toarr_offset));
  ntos_p = ((t_uint32 *) (((char *) obj) + tocount_offset));
  tos = *tos_p;
  ntos = *ntos_p;

  /* copy to the correct section array */
  if (ntos > 0)
    *tos_p = Realloc (tos, sizeof (t_section *) * (ntos + 1));
  else
    *tos_p = Malloc (sizeof (t_section *));
  tos = *tos_p;
  tos[ntos] = sec;
  (*ntos_p) += 1;
  ntos += 1;
  SECTION_SET_TYPE(sec, sectype);

  /* clean up the hole left behind */
  for (i = 0, j = 0; i < nfroms; i++)
    if (froms[i] != sec)
      froms[j++] = froms[i];
  (*nfroms_p) = j;
  if (j == 0)
  {
    Free (froms);
    (*froms_p) = NULL;
  }
} /* }}} */

void *
OverrideSectionType (t_ast_successors * args, void *rule)
{
  t_string section_name = args->args[0]->data.data;
  t_string section_type_string = args->args[1]->data.data;
  char sectype;
  t_section *parent;
  t_section *sub;
  size_t fromarr_offset, fromcount_offset, toarr_offset, tocount_offset;

  if (!strcmp (section_type_string, "CODE"))
    sectype = CODE_SECTION;
  else if (!strcmp (section_type_string, "RODATA"))
    sectype = RODATA_SECTION;
  else if (!strcmp (section_type_string, "DATA"))
    sectype = DATA_SECTION;
  else
    LINKER_SCRIPT_FATAL("Unknown or unsupported section type %s", section_type_string);

  parent = SectionGetFromObjectByName (parser_object, section_name);
  ASSERT(parent, ("Could not find section %s", section_name));
  if (SECTION_TYPE(parent) == sectype)
    return NULL; /* section already has the desired type */

  VERBOSE(1, ("Overriding section type of %s", SECTION_NAME(parent)));

  /* get the offset for the section arrays and counters for the original
   * and new section types. This saves us a lot of trouble when the new types
   * are being assigned {{{ */
  switch (SECTION_TYPE(parent))
  {
    case CODE_SECTION:
      fromarr_offset = ClassOffsetOf (t_object, code);
      fromcount_offset = ClassOffsetOf (t_object, ncodes);
      break;
    case RODATA_SECTION:
      fromarr_offset = ClassOffsetOf (t_object, rodata);
      fromcount_offset = ClassOffsetOf (t_object, nrodatas);
      break;
    case DATA_SECTION:
      fromarr_offset = ClassOffsetOf (t_object, data);
      fromcount_offset = ClassOffsetOf (t_object, ndatas);
      break;
    case BSS_SECTION:
      LINKER_SCRIPT_FATAL("implement");
    default:
      LINKER_SCRIPT_FATAL("section type overriding not supported for this section type");
  }
  switch (sectype)
  {
    case CODE_SECTION:
      toarr_offset = ClassOffsetOf (t_object, code);
      tocount_offset = ClassOffsetOf (t_object, ncodes);
      break;
    case RODATA_SECTION:
      toarr_offset = ClassOffsetOf (t_object, rodata);
      tocount_offset = ClassOffsetOf (t_object, nrodatas);
      break;
    case DATA_SECTION:
      toarr_offset = ClassOffsetOf (t_object, data);
      tocount_offset = ClassOffsetOf (t_object, ndatas);
      break;
    default:
      LINKER_SCRIPT_FATAL("section type overriding not supported for this section type");
  }
  /* }}} */

  SECTION_FOREACH_SUBSECTION (parent, sub)
  {
    VERBOSE(1, (" Moving %s in %s",
                SECTION_NAME(sub), OBJECT_NAME(SECTION_OBJECT(sub))));
    MoveSection (sub, sectype,
                 fromarr_offset, fromcount_offset, toarr_offset, tocount_offset);
  }

  MoveSection (parent, sectype,
               fromarr_offset, fromcount_offset, toarr_offset, tocount_offset);

  return NULL;
}

/* new-style vectorization. if this is sufficient for our purposes, remove
 * old-style vectorization compeletely */

static int
__helper_cmp_sections (const void *a, const void *b)
{
  t_section *seca = *((t_section **) a);
  t_section *secb = *((t_section **) b);

  if (AddressIsGt (SECTION_OLD_ADDRESS(secb), SECTION_OLD_ADDRESS(seca)))
    return -1;
  if (AddressIsLt (SECTION_OLD_ADDRESS(secb), SECTION_OLD_ADDRESS(seca)))
    return 1;
  /* if a section is zero bytes, it has to come after the non-zero byte
   * sections because otherwise the ending address of the vectorized
   * section is calculated wrongly
   */
   if (AddressIsNull (SECTION_CSIZE(secb)))
     return 1;
  
  return -1;
}

static t_section *
DoVectorize (t_section ** subs, t_uint32 nsubs, t_string name)
{
  t_address total_size, start_of_vector, end_of_vector;
  t_section *parent, *vector_sec;
  t_object *linker_object = ObjectGetLinkerSubObject (parser_object);
  t_symbol *sym;
  t_uint32 i;

  if (nsubs == 0)
    return NULL;

  start_of_vector = SECTION_CADDRESS(subs[0]);
  end_of_vector = AddressAdd (
                              SECTION_CADDRESS(subs[nsubs - 1]),
                              SECTION_CSIZE(subs[nsubs - 1]));
  total_size = AddressSub (end_of_vector, start_of_vector);

  parent = SECTION_PARENT_SECTION(subs[0]);
  vector_sec = SectionCreateForObject (
                                       linker_object, SECTION_TYPE(parent),
                                       parent, total_size, name);
  SECTION_SET_CADDRESS(vector_sec, start_of_vector);
  SECTION_SET_OLD_ADDRESS(vector_sec, start_of_vector);
  SECTION_SET_ALIGNMENT(vector_sec, SECTION_ALIGNMENT(subs[0]));

  VERBOSE(1, ("Will vectorize %d subsections (total size @G)", nsubs, total_size));
  VERBOSE(1, ("start @G end @G", start_of_vector, end_of_vector));

  /* {{{ move all data and relocations and symbols */
  for (i = 0; i < nsubs; i++)
  {
    t_section *sub = subs[i];
    t_address offset = AddressSub (SECTION_CADDRESS(sub), start_of_vector);

    VERBOSE(1, ("Subsection %s (%s): offset @G", SECTION_NAME(sub),
                OBJECT_NAME(SECTION_OBJECT(sub)), offset));

    ASSERT (AddressIsLe (AddressAdd (offset, SECTION_CSIZE (sub)),
                         SECTION_CSIZE (vector_sec)),
            ("vectorized subsection out of bounds @G + @G > @G",offset, SECTION_CSIZE (sub), SECTION_CSIZE (vector_sec)));

    /* copy the data */
    if (SECTION_DATA(sub))
    {
      memcpy (((char *) SECTION_DATA(vector_sec)) + AddressExtractUint32 (offset),
              SECTION_DATA(sub), AddressExtractUint32 (SECTION_CSIZE(sub)));
    }

    /* move relocations */
    while (SECTION_REFERS_TO(sub))
    {
      t_reloc *rel = RELOC_REF_RELOC(SECTION_REFERS_TO(sub));

      RelocSetFrom (rel, T_RELOCATABLE(vector_sec));
      RELOC_SET_FROM_OFFSET(rel, AddressAdd (RELOC_FROM_OFFSET(rel), offset));
    }

    while (SECTION_REFED_BY(sub))
    {
      t_reloc *rel = RELOC_REF_RELOC(SECTION_REFED_BY(sub));
      t_uint32 i;
      t_bool done=FALSE;
      for (i=0; i<RELOC_N_TO_SYMBOLS(rel); i++)
      {
        t_symbol *sym = RELOC_TO_SYMBOL(rel)[i];
        if (SYMBOL_BASE(sym)==T_RELOCATABLE(sub))
        {
	  SymbolSetSymbolBaseAndUpdateRelocs (RELOC_TABLE(rel), RELOC_TO_SYMBOL(rel)[i], T_RELOCATABLE(vector_sec));
          SYMBOL_SET_OFFSET_FROM_START(sym, AddressAdd (SYMBOL_OFFSET_FROM_START(sym), offset));
          /* XXX Dominique: this if-test was added to avoid a crash but I am not
           * sure whether this is correct. Quite possibly, all symbols should
           * have MAPPED set at this stage. I just don't know. */
          if (SYMBOL_MAPPED(sym))
          {
            SymbolSetBase(SYMBOL_MAPPED(sym), T_RELOCATABLE(vector_sec));
            SYMBOL_SET_OFFSET_FROM_START(SYMBOL_MAPPED(sym), SYMBOL_OFFSET_FROM_START(sym));
          }
          done = TRUE;
	}
      }
      ASSERT(done, ("Relocs corrupt"));
    }

    while (SECTION_REFED_BY_SYM(sub))
    {
      sym = SECTION_REFED_BY_SYM(sub)->sym;

      SymbolSetBase(sym, T_RELOCATABLE(vector_sec));
      SYMBOL_SET_OFFSET_FROM_START(sym, AddressAdd (SYMBOL_OFFSET_FROM_START(sym), offset));

      if (SYMBOL_MAPPED(sym))
      {
        SymbolSetBase(SYMBOL_MAPPED(sym), T_RELOCATABLE(vector_sec));
        SYMBOL_SET_OFFSET_FROM_START(SYMBOL_MAPPED(sym), SYMBOL_OFFSET_FROM_START(sym));
      }
    }

    /* mark this section for deletion */
    SECTION_SET_IS_VECTORIZED(sub, TRUE);
  }

  /* remove the vectorized subsections */
  for (i = 0; i < nsubs; i++)
  {
    t_section *sub = subs[i];

    if (sub == next_matched_section) 
    { 
        next_matched_section_tel = next_matched_section_tel + 1;
        next_matched_section = ((next_matched_section_tel & 0xfffff) < SECTIONCOUNTBYNUMBER(last_matched_object, next_matched_section_tel)) ? SECTIONARRAYBYNUMBER(last_matched_object, next_matched_section_tel) [next_matched_section_tel & 0xfffff] : NULL;
        next_matched_section_tel --;
    }
    SectionKill(sub);
  }

  return vector_sec;
}

/* takes the name of a parent section, and vectorizes all the subsections of
 * this parent section, in the order they appear in the original executable. */
void *
VectorizeByContents (t_ast_successors * args, void *rule)
{
  t_section **subs;
  t_section *parent;
  t_uint32 nsubs;
  t_address total_size, start_of_vector, end_of_vector;
  t_string tmpname;

  VERBOSE(1, ("Vectorizing section %s", args->args[0]->data.data));

  parent = SectionGetFromObjectByName (parser_object, args->args[0]->data.data);
  ASSERT(parent, ("In vectorizing %s: Could not find parent section to vectorize", args->args[0]->data.data));
  ASSERT(SECTION_TYPE(parent) != BSS_SECTION, ("Implement vectorization of bss"));
  subs = SectionGetSubsections (parent, &nsubs);
  if (!nsubs)
  {
    WARNING(("Section to vectorize has no subsections"));
    return NULL;
  }
  diablo_stable_sort(subs, nsubs, sizeof (t_section *), __helper_cmp_sections);

  if (SECTION_IS_VECTORIZED(subs[0]) ||
      !strncmp (SECTION_NAME(subs[0]), "VECTOR___", 9))
  {
    /* this section has already been vectorized */
    WARNING(("Section %s has already been vectorized", SECTION_NAME(parent)));
    Free (subs);
    return NULL;
  }

  start_of_vector = SECTION_CADDRESS(subs[0]);
  end_of_vector = AddressAdd (
                              SECTION_CADDRESS(subs[nsubs - 1]),
                              SECTION_CSIZE(subs[nsubs - 1]));
  total_size = AddressSub (end_of_vector, start_of_vector);

  /* consistency check: {start|end}_of_vector and total_size should match
   * with the parent section's attributes */
  ASSERT(AddressIsEq (start_of_vector, SECTION_CADDRESS(parent)),
         ("In vectorizing %s: Start addresses don't match up: @G <-> @G",
	  args->args[0]->data.data,
          start_of_vector, SECTION_CADDRESS(parent)));
  ASSERT(AddressIsEq (end_of_vector,
                      AddressAdd (SECTION_CADDRESS(parent), SECTION_CSIZE(parent))),
         ("In vectorizing %s: End addresses don't match up: @G <-> @G", 
	  args->args[0]->data.data,
	  end_of_vector,
          AddressAdd (SECTION_CADDRESS(parent), SECTION_CSIZE(parent))));
  ASSERT(AddressIsEq (total_size, SECTION_CSIZE(parent)),
         ("In vectorizing %s: Sizes don't match up @G <-> @G",
	  args->args[0]->data.data,
          total_size, SECTION_CSIZE(parent)));

  tmpname = StringConcat2 ("VECTOR___", SECTION_NAME(parent));
  DoVectorize (subs, nsubs, tmpname);
  Free (tmpname);

  if (subs)
    Free (subs);
  return NULL;
}

/* Vectorizes all subsections of a given name. The resulting section
 * is again a subsection of the original parent section, but it needn't
 * be the only subsection. */
void *
VectorizeByName (t_ast_successors * args, void *rule)
{
  t_section **subs = NULL;
  t_section *parent;
  t_object *obj, *tmp;
  t_uint32 nsubs, tel;
  t_string name, tmpname;

  name = args->args[0]->data.data;

  VERBOSE(1, ("Vectorizing sections called %s", name));

  parent = NULL;
  nsubs = 0;
  OBJECT_FOREACH_SUBOBJECT(parser_object, obj, tmp)
  {
    t_section * sec;
    OBJECT_FOREACH_SECTION(obj,sec,tel)
    {
      if (!StringPatternMatch(name, SECTION_NAME(sec)))
        continue;

      VERBOSE(1, ("%s %s ----- %s\n", OBJECT_NAME(obj), SECTION_NAME(sec), SECTION_NAME(SECTION_PARENT_SECTION(sec))));

      if (!parent)
        parent = SECTION_PARENT_SECTION(sec);
      else
        ASSERT(parent == SECTION_PARENT_SECTION(sec),
               ("Vectorizing by name (using name %s), but sections come from different parent sections", SECTION_NAME(sec)));
 
      if (nsubs == 0)
        subs = Malloc (sizeof (t_section *));
      else
        subs = Realloc (subs, (nsubs + 1) * sizeof (t_section *));
      subs[nsubs++] = sec;
    }
  }

  if (nsubs == 0)
    return NULL;
  
  diablo_stable_sort (subs, nsubs, sizeof (t_section *), __helper_cmp_sections);

  tmpname = StringConcat2 ("VECTOR___", name);
  DoVectorize (subs, nsubs, tmpname);
  Free (tmpname);

  if (subs)
    Free (subs);
  
  return NULL;
}

/* ADD_SUBSECTION(object_name, parent_section_name, section_name, type, alignment, size) */ 
void *
AddSubsection (t_ast_successors * args, void *rule)
{
  t_object *obj;
  t_section *sec, *parent = NULL;
  t_address x;
  t_ast_node *node;
  t_string preferred_parent = args->args[1]->data.data; 

  obj = ObjectGetFromCache (args->args[0]->data.data, parser_object);
  if (!obj)
  {
    obj = ObjectNewCached (args->args[0]->data.data, parser_object);

    OBJECT_SET_NEXT(obj, NULL);
    if (!OBJECT_MAPPED_LAST(parser_object))
      OBJECT_SET_MAPPED_FIRST(parser_object, obj);
    else
      OBJECT_SET_NEXT(OBJECT_MAPPED_LAST(parser_object), obj);
    OBJECT_SET_MAPPED_LAST(parser_object, obj);
    OBJECT_SET_RELOC_TABLE(obj, RelocTableNew (obj));
    OBJECT_SET_SYMBOL_TABLE(obj, SymbolTableNew (obj));
    OBJECT_SET_SWITCHED_ENDIAN(obj, OBJECT_SWITCHED_ENDIAN(parser_object));
  }

  t_const_string name = args->args[2]->data.data;
  char type;
  if (strcmp (args->args[3]->data.data, "CODE") == 0)
    type = CODE_SECTION;
  else if (strcmp (args->args[3]->data.data, "DATA") == 0)
    type = DATA_SECTION;
  else if (strcmp (args->args[3]->data.data, "RODATA") == 0)
    type = RODATA_SECTION;
  else if (strcmp (args->args[3]->data.data, "BSS") == 0)
    type = BSS_SECTION;
  else
  {
    LINKER_SCRIPT_FATAL("Third argument to function ADD_SUBSECTION should be unquoted CODE, DATA, RODATA or BSS");
    exit (0); /* Keeps the compiler happy */
  }
  sec = SectionNew(obj, type, name);
  SectionInsertInObject (sec, obj);
  SECTION_SET_RELOCATABLE_TYPE(sec, RT_SUBSECTION);

  SECTION_SET_OLD_SIZE(sec, AddressNullForObject (parser_object));

  VERBOSE(1, ("inserted subsection %s %s (%s)", (char *) args->args[0]->data.data,
          (char *) args->args[2]->data.data, (char *) args->args[3]->data.data));

  if ((SECTION_DATA(sec)) && (((t_linker_rule *) rule)->section == NULL))
    LINKER_SCRIPT_FATAL("Rule has no section data!");

  if (((t_linker_rule *) rule)->address == NULL)
    LINKER_SCRIPT_FATAL("Rule has no address for section!");

  if (((t_linker_rule *) rule)->section)
  {
    t_ast_node * r= AstExecute (((t_linker_rule *) rule)->section, FunctionTable, sec,
                current_free_mode);

    if (r) AstNodeFree(r);
    SECTION_SET_CSIZE(sec, args->args[5]->data.addr);
  }
  else
    SECTION_SET_CSIZE(sec, args->args[5]->data.addr);

  if (current_free_mode)
    ((t_linker_rule *) rule)->section = NULL;

  node =
    AstExecute (((t_linker_rule *) rule)->address, AddressTable, NULL,
                current_free_mode);

  if (node->type != TYPE_ADDRESS)
    LINKER_SCRIPT_FATAL("address{} specification of ADD_SUBSECTION does not evaluate to an address");
  x = node->data.addr;
  AstNodeFree (node);
  if (current_free_mode)
    ((t_linker_rule *) rule)->address = NULL;
  SECTION_SET_OLD_ADDRESS(sec, x);
  SECTION_SET_CADDRESS(sec, x);

  /* If we are emulating an existing link, look up the original parent section
   * by looking at the address of the subsection in the linked program. If this
   * address is NULL we won't do the effort to do the lookup. Also, this might go
   * wrong as debug sections have a CADDRESS of NULL.
   */
  if (emulate_link && !AddressIsNull(x))
    parent = SectionGetFromObjectByAddress (parser_object, SECTION_CADDRESS(sec));
 
  if ((!parent) || (!emulate_link))
  {
    /* not emulating an existing link: add to preferred parent section.
     * if this section does not exist, create it */
    parent = SectionGetFromObjectByName (parser_object, preferred_parent);
    if (parent)
    {
      ASSERT (SECTION_TYPE (parent) == SECTION_TYPE (sec),
              ("Adding subsection %s type %c, preferred parent (%s) type %c",
               SECTION_NAME (sec), SECTION_TYPE (sec), SECTION_NAME(parent), SECTION_TYPE (parent)));
      SECTION_SET_CSIZE (parent, AddressAdd (SECTION_CSIZE (parent),
                                             SECTION_CSIZE (sec)));
      SECTION_SET_DATA (parent,
                        Realloc (SECTION_DATA (parent),
				/* We assume no sections larger than 4Gb exist.... */
                                 AddressExtractUint32 (SECTION_CSIZE (parent))));
    }
    else
    {
      parent = SectionCreateForObject (parser_object, SECTION_TYPE (sec), 
                                       NULL, SECTION_CSIZE (sec), preferred_parent);
    }
  }

  ASSERT(parent, ("Parent for linker rule ADD_SUBSECTION not found"));
  MAP_SECTION(sec, SECTION_NAME (parent), parent);

  VERBOSE(1,("Added subsection %s type %c addr @G size @G, preferred parent (%s) type %c addr @G size @G",SECTION_NAME (sec), SECTION_TYPE (sec), SECTION_CADDRESS (sec), SECTION_CSIZE (sec), SECTION_NAME(parent), SECTION_TYPE (parent), SECTION_CADDRESS (parent), SECTION_CSIZE (parent)));
  
  end_of_last_added =
    AddressAdd (SECTION_CADDRESS(sec), SECTION_CSIZE(sec));

  SECTION_SET_ALIGNMENT(sec,
               AddressNewForObject(obj, (t_uint32) (unsigned long) args->args[4]->data.data));

  return NULL;
}

void *
CloneParentAsSubsec (t_ast_successors * args, void *rule)
{
  t_object *obj;
  t_section *sec, *parent = NULL;  
  t_string parentname = args->args[1]->data.data; 

  obj = ObjectGetFromCache (args->args[0]->data.data, parser_object);
  if (!obj)
  {
    obj = ObjectNewCached (args->args[0]->data.data, parser_object);

    OBJECT_SET_NEXT(obj, NULL);
    if (!OBJECT_MAPPED_LAST(parser_object))
      OBJECT_SET_MAPPED_FIRST(parser_object, obj);
    else
      OBJECT_SET_NEXT(OBJECT_MAPPED_LAST(parser_object), obj);
    OBJECT_SET_MAPPED_LAST(parser_object, obj);
    OBJECT_SET_RELOC_TABLE(obj, RelocTableNew (obj));
    OBJECT_SET_SYMBOL_TABLE(obj, SymbolTableNew (obj));
    OBJECT_SET_SWITCHED_ENDIAN(obj, OBJECT_SWITCHED_ENDIAN(parser_object));
  }

  parent = SectionGetFromObjectByName(parser_object,parentname);
  if (!parent)
  {
    LINKER_SCRIPT_FATAL("Could not find section %s", parentname);
    exit (0); /* Keeps the compiler happy */
  }

  sec = SectionCreateForObject(obj, SECTION_TYPE(parent), parent,
                               SECTION_CSIZE(parent), parentname);
  SECTION_SET_ALIGNMENT(sec, SECTION_ALIGNMENT(parent));
  SECTION_SET_CADDRESS(sec, SECTION_CADDRESS(parent));
  SECTION_SET_OLD_ADDRESS(sec, SECTION_OLD_ADDRESS(parent));
  memcpy(SECTION_DATA(sec), SECTION_DATA(parent),
         AddressExtractUint32(SECTION_CSIZE(parent)));
  return NULL;
}

void *
AddLocalSymbol (t_ast_successors * args, void *rule)
{
  t_ast_node *ret;

  if (!((t_linker_rule *) rule)->symbol)
    LINKER_SCRIPT_FATAL("Missing symbol information for ADD_SYMBOL");
  symbol_is_local = TRUE;
  symbol_flags = (t_uint32) (unsigned long) args->args[1]->data.data;
  ret =
    AstExecute (((t_linker_rule *) rule)->symbol, SymbolFunctionTable,
                args->args[0]->data.data, current_free_mode);
  symbol_flags = 0;
  AstNodeFree (ret);
  ((t_linker_rule *) rule)->symbol = NULL;
  return NULL;
}

void *
AddSymbol (t_ast_successors * args, void *rule)
{
  t_ast_node *ret;

  if (!((t_linker_rule *) rule)->symbol)
    LINKER_SCRIPT_FATAL("Missing symbol information for ADD_SYMBOL");
  symbol_is_local = FALSE;
  symbol_flags = (t_uint32) (unsigned long) args->args[1]->data.data;
  ret =
    AstExecute (((t_linker_rule *) rule)->symbol, SymbolFunctionTable,
                args->args[0]->data.data, current_free_mode);
  symbol_flags = 0;
  AstNodeFree (ret);
  ((t_linker_rule *) rule)->symbol = NULL;
  return NULL;
}

void *
AddWeakSymbol (t_ast_successors * args, void *rule)
{
  t_ast_node *ret;

  if (!((t_linker_rule *) rule)->symbol)
    LINKER_SCRIPT_FATAL("Missing symbol information for ADD_SYMBOL");
  symbol_order = 4;
  symbol_is_local = FALSE;
  symbol_dup = PERHAPS;
  symbol_flags = (t_uint32) (unsigned long) args->args[1]->data.data;
  ret =
    AstExecute (((t_linker_rule *) rule)->symbol, SymbolFunctionTable,
                args->args[0]->data.data, current_free_mode);
  symbol_order = 10;
  symbol_dup = FALSE;
  symbol_flags = 0;
  AstNodeFree (ret);
  ((t_linker_rule *) rule)->symbol = NULL;
  return NULL;
}

void *
AddSymbolNew (t_ast_successors * args, void *rule)
{
  t_ast_node *ret;

  if (!((t_linker_rule *) rule)->symbol)
    LINKER_SCRIPT_FATAL("Missing symbol information for ADD_SYMBOL_NEW");
  symbol_is_local = FALSE;
  symbol_order = (t_uint32) (unsigned long) args->args[1]->data.data ;
  symbol_flags = (t_uint32) (unsigned long) args->args[2]->data.data;
  ret =
    AstExecute (((t_linker_rule *) rule)->symbol, SymbolFunctionTable,
                args->args[0]->data.data, current_free_mode);

  symbol_order = 10;
  symbol_flags = 0;

  AstNodeFree (ret);
  ((t_linker_rule *) rule)->symbol = NULL;

  return NULL;
}
/* END ACTION FUNCTIONS }}} */
/* ADDRESS FUNCTIONS {{{ */
void *
Symbol (t_ast_successors * args, void *rule)
{
  t_symbol *sym;
  t_string name = args->args[0]->data.name;
  t_address *ret = Malloc (sizeof (t_address));

  if (strcmp (name, "MATCHED_NAME") == 0)
    LINKER_SCRIPT_FATAL("Deprecated use of MATCHED_NAME. Use MATCHED_NAME() instead");

  sym = SymbolTableGetFirstSymbolWithName (OBJECT_SYMBOL_TABLE(parser_object), name);
  if (!sym)
    LINKER_SCRIPT_FATAL("While executing the address function SYMBOL for linker rule %s: Symbol %s not found!", current_rule_name, name);
  *ret = StackExec(SYMBOL_CODE(sym), NULL, sym, NULL, FALSE, 0, parser_object);
  return ret;
}

void *
SubSymbol (t_ast_successors * args, void *rule)
{
  t_symbol *sym;
  t_string name = args->args[0]->data.name;
  t_address *ret = Malloc (sizeof (t_address));

  if (strcmp (name, "MATCHED_NAME") == 0)
    LINKER_SCRIPT_FATAL("Deprecated use of MATCHED_NAME. Use MATCHED_NAME() instead");

  sym = SymbolTableGetFirstSymbolWithName (OBJECT_SUB_SYMBOL_TABLE(parser_object), name);
  if (!sym)
    LINKER_SCRIPT_FATAL("While executing the address function SUBSYMBOL for linker rule %s: Symbol %s not found!", current_rule_name, name);
  *ret = StackExec(SYMBOL_CODE(sym), NULL, sym, NULL, FALSE, 0, parser_object);
  return ret;
}

void *
AlignSymbol (t_ast_successors * args, void *rule)
{
  t_symbol *sym = (t_symbol *)args->args[0]->data.data;
  t_uint32 align = (t_uint32) (unsigned long) args->args[1]->data.data; 

  t_string code = SYMBOL_CODE(sym);
  t_uint32 adsize = OBJECT_ADDRESS_SIZE(parser_object);

  t_string append;
  t_string newcode;

  VERBOSE(0, ("symbol @S, code %s, align %d", sym, SYMBOL_CODE(sym), align));

  if (adsize == ADDRSIZE32)
  {
    append = StringIo("i%08x+i%08x&$", align-1, ~(t_uint32)(align-1));
  }
  else
  {
    append = StringIo("I%016llx+I%016llx&$", align-1, ~(t_uint64)(align-1));
  }

  StringChop(code, '$');
  newcode = StringConcat2(code, append);
  Free(code);
  Free(append);
  SYMBOL_SET_CODE(sym, newcode);
  return sym;
}

void *
ReadLinkedValue16(t_ast_successors * args, void *rule)
{
	t_section *sec=ObjectGetSectionContainingAddress(parser_object, (t_address ) args->args[0]->data.addr);

        if (!sec) LINKER_SCRIPT_FATAL("In READ_LINKED_VALUE16 for linker rule %s: Could not get data at address @G", current_rule_name, (t_address ) args->args[0]->data.addr);

	return (void *) (unsigned long) SectionGetData16(sec, AddressSub((t_address ) args->args[0]->data.addr, SECTION_CADDRESS(sec)));
}

void *
ReadLinkedValue32(t_ast_successors * args, void *rule)
{
	t_section *sec=ObjectGetSectionContainingAddress(parser_object, (t_address ) args->args[0]->data.addr);

        if (!sec) LINKER_SCRIPT_FATAL("In READ_LINKED_VALUE32 for linker rule %s: Could not get data at address @G", current_rule_name, (t_address ) args->args[0]->data.addr);

	return (void *) (unsigned long) SectionGetData32(sec, AddressSub((t_address ) args->args[0]->data.addr, SECTION_CADDRESS(sec)));
}

void *
ReadLinkedAddr32(t_ast_successors * args, void *rule)
{
	t_section *sec=ObjectGetSectionContainingAddress(parser_object, (t_address ) args->args[0]->data.addr);
        t_address *ret = Malloc (sizeof (t_address));
        
        if (!sec) LINKER_SCRIPT_FATAL("In READ_LINKED_ADDR32 for linker rule %s: Could not get data at address @G", current_rule_name, (t_address ) args->args[0]->data.addr);

        *ret = AddressNewForSection(sec, SectionGetData32(sec, AddressSub((t_address ) args->args[0]->data.addr, SECTION_CADDRESS(sec))));
	return ret;
}

void *
ReadLinkedValue64(t_ast_successors * args, void *rule)
{
	t_section *sec=ObjectGetSectionContainingAddress(parser_object, (t_address ) args->args[0]->data.addr);
        t_address *ret = Malloc (sizeof (t_address));

        if (!sec) LINKER_SCRIPT_FATAL("In READ_LINKED_VALUE32 for linker rule %s: Could not get data at address @G", current_rule_name, (t_address ) args->args[0]->data.addr);

        *ret = AddressNewForSection(sec, SectionGetData64(sec, AddressSub((t_address ) args->args[0]->data.addr, SECTION_CADDRESS(sec))));
	return ret;
}

void *
AbsAddress (t_ast_successors * args, void *rule)
{
  t_address *ret = Malloc (sizeof (t_address));

  *ret = args->args[0]->data.addr;
  return ret;
}

void *
StartOfSectionAddress (t_ast_successors * args, void *name)
{
  t_address *ret = Malloc (sizeof (t_address));
  t_section *sec;

  sec = SectionGetFromObjectByName (parser_object, args->args[0]->data.data);
  if (!sec)
    LINKER_SCRIPT_FATAL("In Start Of Section: section not found!");

  *ret = SECTION_CADDRESS(sec);
  return ret;
}

void *
EndOfSectionAddress (t_ast_successors * args, void *name)
{
  t_address *ret = Malloc (sizeof (t_address));
  t_section *sec;

  sec = SectionGetFromObjectByName (parser_object, args->args[0]->data.data);
  if (!sec)
    LINKER_SCRIPT_FATAL("In End Of Section: section not found!");

  *ret = AddressAdd(SECTION_CADDRESS(sec),SECTION_CSIZE(sec));
  return ret;
}

void *
EndOfLastAdded (t_ast_successors * args, void *rule)
{
  t_address *ret = Malloc (sizeof (t_address));

  *ret = end_of_last_added;
  return ret;
}

/* }}} */
/* SYMBOL FUNCTION {{{ */
void *
StartOfSubsection (t_ast_successors * args, void *name)
{
  t_object *obj;
  t_section *sec;
  t_symbol *sym;

  obj = ObjectGetFromCache (args->args[0]->data.data, parser_object);
  if (!obj)
    LINKER_SCRIPT_FATAL("Executing function Start Of Subsection (symbol): object (%s) not found!", args->args[0]->data.data);
  sec = SectionGetFromObjectByName (obj, args->args[1]->data.data);
  if (!sec)
    LINKER_SCRIPT_FATAL("Executing function Start Of Subsection (symbol): section (%s in %s) not found!", args->args[1]->data.data, args->args[0]->data.data);

  if (symbol_is_local)
    sym =  SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(sec), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);
  else
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+$", symbol_order, symbol_dup, FALSE, T_RELOCATABLE(sec), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);
  DiabloBrokerCall("?LinkerInsertedSymbol",parser_object,OBJECT_SUB_SYMBOL_TABLE(parser_object),sym);
  return sym;
}

void *
StartOfSubsectionWithOffset (t_ast_successors * args, void *name)
{
  t_object *obj;
  t_section *sec;
  t_address offset;
  t_symbol *sym;
  
  obj = ObjectGetFromCache (args->args[0]->data.data, parser_object);
  if (!obj)
    LINKER_SCRIPT_FATAL("Executing function Start Of Subsection With Offset (symbol): object (%s) not found!", args->args[0]->data.data);
  sec = SectionGetFromObjectByName (obj, args->args[1]->data.data);
  if (!sec)
    LINKER_SCRIPT_FATAL("Executing function Start Of Subsection With Offset (symbol): section (%s in %s) not found!", args->args[1]->data.data, args->args[0]->data.data);
  offset = (t_address) args->args[2]->data.addr;

  if (symbol_is_local)
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(sec), offset, AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);
  else
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+$", symbol_order, symbol_dup, FALSE, T_RELOCATABLE(sec), offset, AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);
  DiabloBrokerCall("?LinkerInsertedSymbol",parser_object,OBJECT_SUB_SYMBOL_TABLE(parser_object),sym);
  return sym;
}

void *
EndOfSubsection (t_ast_successors * args, void *name)
{
  t_object *obj;
  t_section *sec;
  t_symbol *sym;

  obj = ObjectGetFromCache (args->args[0]->data.data, parser_object);
  if (!obj)
    LINKER_SCRIPT_FATAL("In Start Of Subsection: object not found!");
  sec = SectionGetFromObjectByName (obj, args->args[1]->data.data);
  if (!sec)
    LINKER_SCRIPT_FATAL("In Start Of Subsection: section not found!");

  if (symbol_is_local)
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+Z00+$", -1, TRUE, FALSE, T_RELOCATABLE(sec), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);
  else
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+Z00+$", symbol_order, symbol_dup, FALSE, T_RELOCATABLE(sec), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);

  DiabloBrokerCall("?LinkerInsertedSymbol",parser_object,OBJECT_SUB_SYMBOL_TABLE(parser_object),sym);
  return sym;
}

void *
EndOfSection (t_ast_successors * args, void *name)
{
  t_section *sec;
  t_symbol *sym;

  sec = SectionGetFromObjectByName (parser_object, args->args[0]->data.data);
  if (!sec)
    LINKER_SCRIPT_FATAL("In End Of Section: section not found!");

  if (symbol_is_local)
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+Z00+$", -1, TRUE, FALSE, T_RELOCATABLE(sec), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);
  else
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+Z00+$", symbol_order, symbol_dup, FALSE, T_RELOCATABLE(sec), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);

  DiabloBrokerCall("?LinkerInsertedSymbol",parser_object,OBJECT_SUB_SYMBOL_TABLE(parser_object),sym);
  return sym;
}

void *
StartOfSection (t_ast_successors * args, void *name)
{
  t_section *sec;
  t_symbol *sym;

  sec = SectionGetFromObjectByName (parser_object, args->args[0]->data.data);
  if (!sec)
    LINKER_SCRIPT_FATAL("In Start Of Section: section %s not found!", args->args[0]->data.data);

  if (symbol_is_local)
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(sec), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);
  else
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+$", symbol_order, symbol_dup, FALSE, T_RELOCATABLE(sec), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);

  DiabloBrokerCall("?LinkerInsertedSymbol",parser_object,OBJECT_SUB_SYMBOL_TABLE(parser_object),sym);
  return sym;
}

void *
SizeOfSection (t_ast_successors * args, void *name)
{
  t_section *sec;
  t_address *ret = Malloc (sizeof (t_address));


  sec = SectionGetFromObjectByName (parser_object, args->args[0]->data.data);
  if (!sec)
    LINKER_SCRIPT_FATAL("In Size Of Section: section %s not found!", args->args[0]->data.data);

  *ret=SECTION_CSIZE(sec);
  return ret;
}

void *
SizeOfSubsection (t_ast_successors * args, void *name)
{
  t_object *obj;
  t_section *sec;
  t_symbol *sym;

  obj = ObjectGetFromCache (args->args[0]->data.data, parser_object);
  if (!obj)
    LINKER_SCRIPT_FATAL("In Start Of Subsection: object not found!");
  sec = SectionGetFromObjectByName (obj, args->args[1]->data.data);
  if (!sec)
    LINKER_SCRIPT_FATAL("In Start Of Subsection: section (%s in %s) not found!", args->args[1]->data.data, args->args[0]->data.data);

  if (symbol_is_local)
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "Z00$", -1, TRUE, FALSE, T_RELOCATABLE(sec), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);
  else
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "Z00$", symbol_order, symbol_dup, FALSE, T_RELOCATABLE(sec), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);

  DiabloBrokerCall("?LinkerInsertedSymbol",parser_object,OBJECT_SUB_SYMBOL_TABLE(parser_object),sym);
  return sym;
}

void *
Offset (t_ast_successors * args, void *name)
{
  return args->args[0]->data.data;
}

void *
AbsSymbol (t_ast_successors * args, void *name)
{
  t_symbol *sym;

  if (symbol_is_local)
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(parser_object)), args->args[0]->data.addr, AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);
  else
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+$", symbol_order, symbol_dup, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(parser_object)), args->args[0]->data.addr, AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);

  DiabloBrokerCall("?LinkerInsertedSymbol",parser_object,OBJECT_SUB_SYMBOL_TABLE(parser_object),sym);
  return sym;
}

void *
UndefSymbol (t_ast_successors * args, void *name)
{
  t_symbol *sym;

  if (symbol_is_local)
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+$", -1, TRUE, TRUE, T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);
  else
    sym = SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(parser_object), name, "R00A00+$", symbol_order, symbol_dup, TRUE, T_RELOCATABLE(OBJECT_UNDEF_SECTION(parser_object)), AddressNullForObject(parser_object), AddressNullForObject(parser_object), NULL, AddressNullForObject(parser_object), symbol_flags);

  DiabloBrokerCall("?LinkerInsertedSymbol",parser_object,OBJECT_SUB_SYMBOL_TABLE(parser_object),sym);
  return sym;
}

void *
DuplicateExistingSymbol (t_ast_successors * args, void *name)
{
  t_symbol *sym, *dup;
  t_string origname = args->args[0]->data.data;

  VERBOSE(1, ("Duplicating %s as %s", origname, name));

  sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(parser_object), origname);
  if (!sym)
    LINKER_SCRIPT_FATAL("Could not find original symbol %s", origname);

  dup = SymbolTableDupSymbolWithOrder(OBJECT_SUB_SYMBOL_TABLE(parser_object), sym, name, symbol_order);
  SYMBOL_SET_DUP(dup, symbol_dup);

  return dup;
}

void *
DuplicateOriginalSymbol (t_ast_successors * args, void *name)
{
  t_symbol *sym, *dup;
  t_string origname = args->args[0]->data.data;

  VERBOSE(1, ("Duplicating %s as %s", origname, name));

  sym = SymbolTableGetSymbolByName (OBJECT_SYMBOL_TABLE(parser_object), origname);
  if (!sym)
    LINKER_SCRIPT_FATAL("Could not find original symbol %s", origname);

  dup = SymbolTableDupSymbolWithOrder(OBJECT_SUB_SYMBOL_TABLE(parser_object), sym, name, symbol_order);
  SYMBOL_SET_DUP(dup, symbol_dup);

  return dup;
}

/*}}} */

void *
UtilConcat (t_ast_successors * args, void *name)
{
  return StringConcat2 (args->args[0]->data.data, args->args[1]->data.data);
}

void *
MatchedName (t_ast_successors * args, void *name)
{
  if (!last_matched_symbol_name) 
    LINKER_SCRIPT_FATAL("Linker rule (%s) called MATCHED_NAME, but last_matched_symbol_name is not set!", current_rule_name);

  return StringDup (last_matched_symbol_name);
}

void *
MatchedSymbolFlags(t_ast_successors * args, void *name)
{
  if (!last_matched_symbol) 
    LINKER_SCRIPT_FATAL("Linker rule called MATCHED_SYMBOL_FLAGS, but last_matched_symbol is not set!");

  return (void*)(long)SYMBOL_FLAGS(last_matched_symbol);
}

void *
MatchedSymbolSize (t_ast_successors * args, void *name)
{
  t_address *ret = Malloc (sizeof (t_address));
  if (!last_matched_symbol) 
    LINKER_SCRIPT_FATAL("Linker rule called MATCHED_SYMBOL_SIZE, but last_matched_symbol is not set!");

  *ret= SYMBOL_SIZE(last_matched_symbol);

  return ret;
}

void *
MatchedSymbolValue (t_ast_successors * args, void *name)
{
  t_address *ret = Malloc (sizeof (t_address));
  if (!last_matched_symbol) 
    LINKER_SCRIPT_FATAL("Linker rule called MATCHED_SYMBOL_VALUE, but last_matched_symbol is not set!");

  *ret= StackExec(SYMBOL_CODE(last_matched_symbol), NULL, last_matched_symbol, NULL, FALSE, 0, parser_object);

  return ret;
}

void *
MatchedSubobjectName (t_ast_successors * args, void *name)
{
  if (last_matched_object)
    return StringDup (OBJECT_NAME(last_matched_object));
  return NULL;
}

void *
MatchedSubsectionName (t_ast_successors * args, void *name)
{
  if (last_matched_section)
    return StringDup (SECTION_NAME(last_matched_section));
  return NULL;
}

void *
UtilSubString (t_ast_successors * args, void *name)
{
  t_string in = args->args[0]->data.data;
  t_uint32 from = (t_uint32) (unsigned long) args->args[1]->data.data;
  t_uint32 to = (t_uint32) (unsigned long) args->args[2]->data.data;
  t_string ret = StringDup (in + from);

  if (to)
    ret[to] = '\0';
  return ret;
}

void *
UtilSubStringToken (t_ast_successors * args, void *name)
{
  t_string in = args->args[0]->data.data;
  t_string sep = args->args[1]->data.data;
  t_uint32 part = (t_uint32) (unsigned long) args->args[2]->data.data;
  t_string_array * array = StringDivide(in, sep, FALSE, FALSE);

  t_string_array_elem * out;
  t_string outs;
  t_uint32 i = 0;
 
  STRING_ARRAY_FOREACH_ELEM(array, out)
  {
    if (i == part) 
      break;
    i++;
  }

  ASSERT(out, ("Token %d not found", part));


  outs = StringDup(out->string);
  StringArrayFree(array);
  return outs;
}

void *
UtilSubStringTokenReverse (t_ast_successors * args, void *name)
{
  t_string in = args->args[0]->data.data;
  t_string sep = args->args[1]->data.data;
  t_uint32 part = (t_uint32) (unsigned long) args->args[2]->data.data;
  t_string_array * array = StringDivide(in, sep, FALSE, FALSE);

  t_string_array_elem * out;
  t_string outs;
  t_uint32 i = 0;

  STRING_ARRAY_FOREACH_ELEM_R(array, out)
  {
    if (i == part)
      break;
    i++;
  }

  ASSERT(out, ("Token %d not found", part));


  outs = StringDup(out->string);
  StringArrayFree(array);
  return outs;
}


void *
UtilStrlen (t_ast_successors * args, void *name)
{
  t_string in = args->args[0]->data.data;
  return (void *) strlen(in);
}

void *
UtilSignExtend (t_ast_successors * args, void *name)
{
  t_address addr = (t_address) args->args[1]->data.addr;
  t_uint32 signbit = (t_uint32) (unsigned long) args->args[0]->data.data;
  t_uint64 extended = AddressExtractUint64 (addr);
  t_uint64 mask = (((t_uint64)1) << signbit) - 1;
  t_address *ret = Malloc (sizeof (t_address));
  
  if (extended & (t_uint64)(1 << signbit))
    extended = extended | ~mask; /* negative */
  else
    extended = extended & mask;  /* positive */

  /* ugly way to make an address of the same type as addr */
  *ret = AddressAddUint64 (AddressSub (addr, addr), extended);
  return ret;
}


void *
UtilUint32ToAddr (t_ast_successors * args, void *name)
{
  t_uint32 value = (t_uint32) (unsigned long) args->args[0]->data.data;
  t_address *ret = Malloc (sizeof (t_address));

  *ret = AddressNew32 (value);
  return ret;
}



/* The tables describing all callbacks {{{*/
/* Types:
 *
 * A: Address
 * B: Bool
 * E: sEction
 * O: other ((unquoted) string that's casted to a specific type by the called function)
 * S: String
 * T: Trigger: 0 = trigger false, 1 = trigger true, 2 = trigger false, but needs another run, 3 = trigger true and needs another run
 * V: Void
 * Y: sYmbol
 */

t_ast_node_table_entry FunctionTable[] = {
  /* Trigger (boolean) functions */
  {"ALWAYS", 0, NULL, "T", TriggerAlways}
  ,
    {"UNDEFINED_SYMBOL", 1, "S", "T", TriggerUndefinedSymbol}
  ,
    {"WEAK_UNDEFINED_SYMBOL", 1, "S", "T", TriggerWeakUndefinedSymbol}
  ,
    {"SYMBOL_EXISTS", 1, "S", "T", TriggerSymbolExists}
  ,
    {"LINKED_SYMBOL_EXISTS", 1, "S", "T", TriggerLinkedSymbolExists}
  ,
    {"UNDEF_OR_NONEXIST_SYMBOL", 1, "S", "T", TriggerUndefOrNonExistSymbol}
  ,
    {"SUBSECTION_EXISTS", 2, "SS", "T", TriggerSubsectionExists}
  ,
    {"SECTION_EXISTS", 1, "S", "T", TriggerSectionExists}
  ,
    {"GENERATE_PIC", 0, "", "T", TriggerGeneratePIC}
  ,
    /* Filler functions */
    {"ADDRESS", 1, "A", "V", FillerAddress}
  ,
    {"CONST64", 1, "N", "V", FillerConst64}
  ,
    {"CONST32", 1, "N", "V", FillerConst32}
  ,
    {"CONST16", 1, "N", "V", FillerConst16}
  ,
    {"CONST8", 1, "N", "V", FillerConst8}
  ,
    {"STRING", 1, "S", "V", FillerString}
  ,
    {"RELOCATED0", 6, "NSNSNS", "V", FillerRelocated0}
  ,
    {"RELOCATED32", 6, "NSNSNS", "V", FillerRelocated32}
  ,
    {"RELOCATED64", 6, "NSNSNS", "V", FillerRelocated64}
  ,
    {"START_OF_SECTION", 1, "S", "V", FillerStartOfSection}
  ,
    {"END_OF_SECTION", 1, "S", "V", FillerEndOfSection}
  ,
    {"SIZE_OF_SECTION", 1, "S", "V", FillerSizeOfSection}
  ,
    {"SECTION_OFFSET", 2, "EA", "V", FillerSectionOffset }   
  ,                                                     
    {"SUBSECTION_BY_ADDRESS", 1, "A", "E", GetSubSectionContainingAddress } 
  ,                                                   
    /* Action functions */
    {"ADD_SUBSECTION", 6, "SSSONA", "E", AddSubsection}
  ,
    {"CLONE_PARENT_AS_SUBSECTION", 2, "SS", "E", CloneParentAsSubsec}
  ,
    {"ADD_SYMBOL", 2, "SN", "Y", AddSymbol}
  ,
    {"ADD_WEAK_SYMBOL", 2, "SN", "Y", AddWeakSymbol}
  ,
    {"ADD_SYMBOL_NEW", 3, "SNN", "Y", AddSymbolNew}
  ,
    {"ADD_LOCAL_SYMBOL", 2, "SN", "Y", AddLocalSymbol}
  ,
    {"VECTORIZE_BY_NAME", 1, "S", "E", VectorizeByName}
  ,
    {"VECTORIZE_BY_CONTENTS", 1, "S", "E", VectorizeByContents}
  ,
    {"VECTORIZE", 1, "S", "E", VectorizeByContents}
  ,
    {"OVERRIDE_SECTION_TYPE", 2, "SO", "E", OverrideSectionType}
  ,
    {"STRINGCHOP", 2, "SS", "E", SectionStringChop}
  ,
    {"READ_LINKED_VALUE16", 1, "A", "N", ReadLinkedValue16}
  ,
    {"READ_LINKED_VALUE32", 1, "A", "N", ReadLinkedValue32}
  ,
    {"READ_LINKED_VALUE64", 1, "A", "A", ReadLinkedValue64}
  ,
    {"READ_LINKED_ADDR32", 1, "A", "A", ReadLinkedAddr32}
  ,
    {"SYMBOL", 1, "S", "A", Symbol}
  ,
    {NULL, 0, NULL, NULL, NULL}
};

t_ast_node_table_entry AddressTable[] = {
  /* Address functions */
    {"SYMBOL", 1, "S", "A", Symbol}
  ,
    {"SUBSYMBOL", 1, "S", "A", SubSymbol}
  ,
    {"ABS", 1, "A", "A", AbsAddress}
  ,
    {"START_OF_SECTION", 1, "S", "A", StartOfSectionAddress}
  ,
    {"END_OF_SECTION", 1, "S", "A", EndOfSectionAddress}
  ,
    {"END_OF_LAST_ADDED", 0, NULL, "A", EndOfLastAdded}
  ,
    {"READ_LINKED_VALUE16", 1, "A", "N", ReadLinkedValue16}
  ,
    {"READ_LINKED_VALUE32", 1, "A", "N", ReadLinkedValue32}
  ,
    {"READ_LINKED_ADDR32", 1, "A", "A", ReadLinkedAddr32}
  ,
    {"READ_LINKED_VALUE64", 1, "A", "A", ReadLinkedValue64}
  ,
    {NULL, 0, NULL, NULL, NULL}
};

/* Symbol functions */
t_ast_node_table_entry SymbolFunctionTable[] = {
  {"START_OF_SUBSECTION", 2, "SS", "Y", StartOfSubsection},
  {"START_OF_SUBSECTION_OFFSET", 3, "SSA", "Y", StartOfSubsectionWithOffset},
  {"END_OF_SUBSECTION", 2, "SS", "Y", EndOfSubsection},
    {"SIZE_OF_SUBSECTION", 2, "SS", "Y", SizeOfSubsection}
  ,
    {"ABS", 1, "A", "Y", AbsSymbol}
  ,
    {"OFFSET", 1, "N", "N", Offset}
  ,
    {"START_OF_SECTION", 1, "S", "Y", StartOfSection}
  ,
    {"SIZE_OF_SECTION", 1, "S", "A", SizeOfSection}
  ,
    {"END_OF_SECTION", 1, "S", "Y", EndOfSection}
  ,
    {"DUPLICATE", 1, "S", "Y", DuplicateExistingSymbol}
  ,
    {"DUPLICATE_ORIG", 1, "S", "Y", DuplicateOriginalSymbol}
  ,
  {"SYMBOL", 1, "S", "A", Symbol}
  ,
  {"ALIGN", 2, "YN", "Y", AlignSymbol}
  ,
    {"UNDEFINED", 0, "", "Y", UndefSymbol}
  ,
    {NULL, 0, NULL, NULL, NULL}
};

t_ast_node_table_entry UtilityFunctions[] = {
  {"CONCAT", 2, "SS", "S", UtilConcat}
  ,
    {"MATCHED_NAME", 0, NULL, "S", MatchedName}
  ,
    {"MATCHED_SYMBOL_FLAGS", 0, NULL, "N", MatchedSymbolFlags}
  ,
    {"MATCHED_SYMBOL_SIZE", 0, NULL, "A", MatchedSymbolSize}
  ,   
    {"MATCHED_SYMBOL_VALUE", 0, NULL, "A", MatchedSymbolValue}
  ,
    {"MATCHED_SUBSECTION_NAME", 0, NULL, "S", MatchedSubsectionName}
  ,
    {"MATCHED_SUBOBJECT_NAME", 0, NULL, "S", MatchedSubobjectName}
  ,
    {"STRLEN", 1, "S", "N", UtilStrlen}
  ,
    {"SUBSTRING", 3, "SNN", "S", UtilSubString}
  ,
    {"STRINGTOKEN", 3, "SSN", "S", UtilSubStringToken}
  ,
    {"STRINGTOKENR", 3, "SSN", "S", UtilSubStringTokenReverse}
  ,
    {"SIGN_EXTEND", 2, "NA", "A", UtilSignExtend}
  ,
    {"UINT32_TO_ADDR", 1, "N", "A", UtilUint32ToAddr}
  ,
    {NULL, 0, NULL, NULL, NULL}
};

/* this table contains callback functions installed by architecture handlers */
t_ast_node_table_entry *InstalledCallbacksTable = NULL;
t_uint32 InstalledCallbacksTableCount = 0;
/*}}}*/

/* }}} */
/* }}} Callbacks for AST Execution */

/* {{{ linker script callback function management */
void LinkerScriptInstallCallback(t_string name, t_uint32 nargs, t_string args, t_string returntype, void *(*fun)(t_ast_successors *, void *))
{
  t_ast_node_table_entry *new;
  if (!InstalledCallbacksTable)
  {
    new = InstalledCallbacksTable = Calloc(2, sizeof(t_ast_node_table_entry));
  }
  else
  {
    int ninstalled = 1;
    t_ast_node_table_entry *entry = InstalledCallbacksTable;
    while (entry->name)
    {
      ++ninstalled;
      ++entry;
    }
    InstalledCallbacksTable =
      Realloc(InstalledCallbacksTable,
              (ninstalled+1)*sizeof(t_ast_node_table_entry));
    /* zero terminating entry */
    entry = InstalledCallbacksTable + ninstalled;
    entry->name = entry-> arguments = entry->ret = NULL;
    entry->fun = NULL;
    entry->nargs = 0;

    new = InstalledCallbacksTable + (ninstalled-1);
  }

  new->name = StringDup(name);
  new->nargs = nargs;
  new->arguments = args;
  new->ret = returntype;
  new->fun = fun;

  InstalledCallbacksTableCount++;
}
/* }}} */

void LinkerScriptUninstallCallbacks()
{
    t_uint32 i;
    for (i = 0; i < InstalledCallbacksTableCount; i++)
    {
      Free(InstalledCallbacksTable[i].name);
    }

    Free(InstalledCallbacksTable);
}


/* AST Execution {{{ */

/* The following functions implement the real execution of the AST's. They use
 * the functions in the tables, and knowledge about operators to collapse an
 * AST into one atomic ast node. */

t_ast_node_table_entry *
FunctionTableGetFunction (t_ast_node_table_entry * function_table,
                          t_string name)
{
  t_ast_node_table_entry *fun = function_table;

  while (fun->name)
  {
    if (strcmp (fun->name, name) == 0)
    {
      return fun;
    }
    fun++;
  }

  /* if the requested function is not found here, try the callbacks table */
  if (InstalledCallbacksTable)
  {
    fun = InstalledCallbacksTable;
    while (fun->name)
    {
      if (strcmp (fun->name, name) == 0)
      {
        return fun;
      }
      fun++;
    }
  }

  return NULL;
}

/* AstExecute {{{ */
/*! This function recursively collapses the arguments of the node, and then the
 * node itself. */

t_ast_node *
AstExecuteREAL(const char *file, int lnno, t_ast_node * in,
               t_ast_node_table_entry * function_table,
               void *implicit_argument, t_bool free_mode)
{
  VERBOSE(1,("##traceback: call from %s:%d", file, lnno));
  current_free_mode = free_mode;
  /* If we have a function */
  if (in->type == TYPE_FUNCTION)
  {
    t_ast_successors *args = in->args;
    t_string name = in->data.name;
    t_ast_node_table_entry *fun =
      FunctionTableGetFunction (function_table, name);
    void *tmp;
    t_ast_node *ret = Malloc (sizeof (t_ast_node));

    if (!fun)
      fun = FunctionTableGetFunction (UtilityFunctions, name);
    if (!fun)
      LINKER_SCRIPT_FATAL("Function %s (call at %s %d) not found!", name, file, lnno);
    if (fun->fun == NULL)
      LINKER_SCRIPT_FATAL("Function %s is not yet implemented", name);

    CheckAndCastArguments (in, fun->nargs, fun->arguments, function_table,
                           implicit_argument);

    tmp = fun->fun (args, implicit_argument);
    AstNodeFree (in);
    ret->args = NULL;
    switch (fun->ret[0])
    {
      case 'V':
        ret->type = TYPE_VOID;
        break;
      case 'T':
        ret->type = TYPE_TRIGGER;
        ret->data.data = tmp;
        break;
      case 'B':
        ret->type = TYPE_BOOL;
        ret->data.data = tmp;
        break;
      case 'A':
        ret->type = TYPE_ADDRESS;
        ret->data.addr = *((t_address *) tmp);
        Free (tmp);
        break;
      case 'E':
        ret->type = TYPE_SECTION;
        ret->data.data = tmp;
        break;
      case 'Y':
        ret->type = TYPE_SYMBOL;
        ret->data.data = tmp;
        break;
      case 'N':
        ret->type = TYPE_NUMERIC;
        ret->data.data = tmp;
        break;
      case 'S':
        ret->type = TYPE_STRING;
        ret->data.data = tmp;
        break;
      default:
        LINKER_SCRIPT_FATAL("Implement type %c\n", fun->ret[0]);
    }
    return ret;
  }
  /* If we have an operator */
  else if (in->type == TYPE_OPER)
  {
    switch (in->data.oper)
    {
      case OPER_COMMA:
        CheckAndCastOneArgument ("COMMA", 0, &(in->args->args[0]), 'V',
                                 function_table, implicit_argument);
        CheckAndCastOneArgument ("COMMA", 1, &(in->args->args[1]), 'V',
                                 function_table, implicit_argument);
        AstNodeFree (in);
        return NULL;
        break;
      case OPER_AND:
        {
          t_ast_node *ret = Malloc (sizeof (t_ast_node));

          ret->type = TYPE_TRIGGER;

          CheckAndCastOneArgument ("AND", 0, &(in->args->args[0]), 'T',
                                   function_table, implicit_argument);
          
          if (!(((t_uint32) (unsigned long) in->args->args[0]->data.data) & 1))
          {
            ret->data.data = ((void *) (unsigned long) (((t_uint32) (unsigned long) in->args->args[0]->data.data)  & (~1)));
            ret->args = NULL;
            AstNodeFree (in);
            return ret;
          }
          else
          {
            CheckAndCastOneArgument ("AND", 1, &(in->args->args[1]), 'T',
                                     function_table, implicit_argument);

            if (!(((t_uint32) (unsigned long) in->args->args[1]->data.data) & 1))
            {
              ret->data.data = (void *) (unsigned long) ((((t_uint32) (unsigned long) in->args->args[0]->data.data) | ((t_uint32) (unsigned long) in->args->args[1]->data.data)) & (~1));
              ret->args = NULL;
              AstNodeFree (in);
              return ret;
            }
            else
            {
              ret->data.data = (void *) (unsigned long) (((t_uint32) (unsigned long) in->args->args[0]->data.data | (t_uint32) (unsigned long) in->args->args[1]->data.data));
              ret->args = NULL;
              AstNodeFree (in);
              return ret;
            }
          }

          LINKER_SCRIPT_FATAL("Unreachable!");
          break;
        }
      case OPER_ADD:
      case OPER_SUB:
      case OPER_BITWISE_AND:
      case OPER_DIV:
      case OPER_SHIFT_RIGHT:
      case OPER_SHIFT_LEFT:
        {
          t_ast_node *ret = Malloc (sizeof (t_ast_node));
          t_ast_node *left = in->args->args[0];
          t_ast_node *right = in->args->args[1];


          if (left->type == TYPE_FUNCTION)
	  {
		  int backup_free_mode=current_free_mode;
		  in->args->args[0] = left =
			  AstExecute (left, function_table, implicit_argument, FALSE);
		  current_free_mode=backup_free_mode;
          }

          if (right->type == TYPE_FUNCTION)
          {
		  int backup_free_mode=current_free_mode;
            in->args->args[1] = right =
              AstExecute (right, function_table, implicit_argument,
                          FALSE);
		  current_free_mode=backup_free_mode;
          }

	  if (left->type == TYPE_OPER)
	  {
		  int backup_free_mode=current_free_mode;
		   in->args->args[0] = left =
			   AstExecute (left, function_table, implicit_argument, FALSE);
		  current_free_mode=backup_free_mode;
	  }

	  if (right->type == TYPE_OPER)
	  {
		  int backup_free_mode=current_free_mode;
		   in->args->args[1] = right =
			   AstExecute (right, function_table, implicit_argument, FALSE);
		  current_free_mode=backup_free_mode;
	  }

	  if (left->type == TYPE_OTHER)
	  {
            t_string orig = left->data.data;

            left->type = TYPE_NUMERIC;
            left->data.data =
              (void *) (unsigned long) StringToUint64 (orig, 0);
            Free (orig);
	  }

          if (right->type == TYPE_OTHER)
          {
            t_string orig = right->data.data;

            right->type = TYPE_NUMERIC;
            right->data.data =
              (void *) (unsigned long) StringToUint64 (orig, 0);
            Free (orig);
          }
	 
	  if ((left->type == TYPE_SYMBOL) && (right->type == TYPE_NUMERIC))
          {
            t_symbol *sym = left->data.data;

	    if (in->data.oper==OPER_SUB)
	    {
            SYMBOL_SET_ADDEND(sym, AddressSubUint32 (SYMBOL_ADDEND(sym), (t_uint32) (unsigned long) right->data.data));
	    }
	    else if (in->data.oper==OPER_ADD)
	    {
            SYMBOL_SET_ADDEND(sym, AddressAddUint32 (SYMBOL_ADDEND(sym), (t_uint32) (unsigned long) right->data.data));
	    }
	    else LINKER_SCRIPT_FATAL("Implement");

            
            ret->type = TYPE_SYMBOL;
            ret->data.data = sym;
            ret->args = NULL;
          }
	  else if ((left->type == TYPE_SYMBOL) && (right->type == TYPE_ADDRESS))
          {
            t_symbol *sym = left->data.data;

	    if (in->data.oper==OPER_SUB)
	    {
            SYMBOL_SET_ADDEND(sym, AddressSub (SYMBOL_ADDEND(sym), right->data.addr));
	    }
	    else if (in->data.oper==OPER_ADD)
	    {
            SYMBOL_SET_ADDEND(sym, AddressAdd (SYMBOL_ADDEND(sym), right->data.addr));
	    }
	    else LINKER_SCRIPT_FATAL("Implement");
            ret->type = TYPE_SYMBOL;
            ret->data.data = sym;
            ret->args = NULL;
          }
          else if ((left->type == TYPE_NUMERIC) && (right->type == TYPE_NUMERIC))
	  {
            ret->type = TYPE_NUMERIC;
	    if (in->data.oper==OPER_SUB)
	    {
              ret->data.data = (void *) (unsigned long) ((t_uint32) (unsigned long) left->data.data - (t_uint32) (unsigned long) right->data.data);
	    }
	    else if (in->data.oper==OPER_ADD)
	    {
              ret->data.data = (void *) (unsigned long) ((t_uint32) (unsigned long) left->data.data + (t_uint32) (unsigned long) right->data.data);
	    }
            else if (in->data.oper==OPER_SHIFT_RIGHT)
            {
              ret->data.data = (void *) (unsigned long) ((t_uint32) (unsigned long) left->data.data >> (t_uint32) (unsigned long) right->data.data);
            }
            else if (in->data.oper==OPER_SHIFT_LEFT)
            {
              ret->data.data = (void *) (unsigned long) ((t_uint32) (unsigned long) left->data.data << (t_uint32) (unsigned long) right->data.data);
            }
            else if (in->data.oper==OPER_BITWISE_AND)
            {
              ret->data.data = (void *) (unsigned long) ((t_uint32) (unsigned long) left->data.data & (t_uint32) (unsigned long) right->data.data);
            }
            else if (in->data.oper==OPER_DIV)
            {
              ret->data.data = (void *) (unsigned long) ((t_uint32) (unsigned long) left->data.data / (t_uint32) (unsigned long) right->data.data);
            }
	    else LINKER_SCRIPT_FATAL("Implement linker operand %d on numeric values",in->data.oper);
            ret->args = NULL;
	  }
	  else if ((left->type == TYPE_ADDRESS) && (right->type == TYPE_ADDRESS))
	  {
            ret->type = TYPE_ADDRESS;
	    if (in->data.oper==OPER_SUB)
	    {
            ret->data.addr = AddressSub(left->data.addr, right->data.addr);
	    }
	    else if (in->data.oper==OPER_ADD)
	    {
            ret->data.addr = AddressAdd(left->data.addr, right->data.addr);
	    }
            else if (in->data.oper==OPER_SHIFT_RIGHT)
            {
              ret->data.addr = AddressNewForObject(parser_object,
                                                  AddressExtractUint64(left->data.addr) >> AddressExtractUint32(right->data.addr));
            }
            else if (in->data.oper==OPER_SHIFT_LEFT)
            {
              ret->data.addr = AddressNewForObject(parser_object,
                                                  AddressExtractUint64(left->data.addr) << AddressExtractUint32(right->data.addr));
            }
            else if (in->data.oper==OPER_BITWISE_AND)
            {
              ret->data.addr = AddressNewForObject(parser_object,
                                                  AddressExtractUint64(left->data.addr) & AddressExtractUint32(right->data.addr));
            }
            else if (in->data.oper==OPER_DIV)
            {
              ret->data.addr = AddressNewForObject(parser_object,
                                                  AddressExtractUint64(left->data.addr) / AddressExtractUint32(right->data.addr));
            }
	    else 
              LINKER_SCRIPT_FATAL("Implement linker operand %d on addresses", in->data.oper);
            ret->args = NULL;
	  }
          else if ((left->type == TYPE_ADDRESS) && (right->type == TYPE_NUMERIC))
	  {
            ret->type = TYPE_ADDRESS;
	    if (in->data.oper==OPER_SUB) /* OPER 3 */
	    {
              ret->data.addr = AddressSubUint32(left->data.addr, (t_uint32) (unsigned long) right->data.data);
	    }
	    else if (in->data.oper==OPER_ADD)
	    {
              ret->data.addr = AddressAddUint32(left->data.addr, (t_uint32) (unsigned long) right->data.data);
	    }
	    else if (in->data.oper==OPER_BITWISE_AND)
            {
              ret->data.addr = AddressAnd(left->data.addr, AddressNewForObject(parser_object,(t_uint32) (unsigned long) right->data.data));
	    }
            else if (in->data.oper==OPER_SHIFT_LEFT) /* OPER 6 */
            {
              ret->data.addr = AddressNewForObject(parser_object,
                                                   AddressExtractUint64(left->data.addr) << ((t_uint64) (unsigned long) right->data.data));
            }
	    else if (in->data.oper==OPER_DIV)
	    {
              ret->data.addr = AddressDivUint32(left->data.addr, (t_uint32) (unsigned long) right->data.data);
	    }
	    else LINKER_SCRIPT_FATAL("Implement linker operand %d on address/numeric combination", in->data.oper);
            ret->args = NULL;
	  }
	  else if ((left->type == TYPE_NUMERIC) && (right->type == TYPE_ADDRESS))
	  {
            ret->type = TYPE_ADDRESS;
	    if (in->data.oper==OPER_SUB)
	    {
              ret->data.addr = AddressSub(AddressNewForObject(parser_object, ((t_uint64) (unsigned long) left->data.data)), right->data.addr);
	    }
	    else if (in->data.oper==OPER_ADD)
	    {
              ret->data.addr = AddressAddUint32(right->data.addr, (t_uint32) (unsigned long) left->data.data);
	    }
	    else if (in->data.oper==OPER_BITWISE_AND)
            {
              ret->data.addr = AddressAnd(right->data.addr, AddressNewForObject(parser_object,(t_uint32) (unsigned long) left->data.data));
	    }
	    else if (in->data.oper==OPER_DIV)
	    {
              ret->data.addr = AddressDivUint32(right->data.addr, (t_uint32) (unsigned long) left->data.data);
	    }
	    else if (in->data.oper==OPER_SHIFT_RIGHT)
            {
              ret->data.addr = AddressNewForObject(parser_object, (t_uint32) (unsigned long) left->data.data >> AddressExtractUint32(right->data.addr));
            }
            else LINKER_SCRIPT_FATAL("Implement");
            ret->args = NULL;
	  }
	  else
          {
            LINKER_SCRIPT_FATAL("Unknown add %d %d\n", left->type, right->type);
          }

          AstNodeFree (in);
          return ret;
          break;

        }
      case OPER_OR:
        {

          /* THIS IS SOMEWHAT UGLY, but neither subclauses of an OR change the last_matched_symbol or the last_matched_symbol_name */
          /* The reason is that it is quite impossible to decide which of the two clauses should be used later on */

          t_ast_node *ret = Malloc (sizeof (t_ast_node));

          t_string prev_matched_symbol_name = StringDup (last_matched_symbol_name);
          t_symbol *prev_matched_symbol = last_matched_symbol;

          ret->type = TYPE_TRIGGER;

          CheckAndCastOneArgument ("OR", 0, &(in->args->args[0]), 'T',
                                   function_table, implicit_argument);
          
          if (!(((t_uint32) (unsigned long) in->args->args[0]->data.data) & 1))
            {

              if (last_matched_symbol_name)
                {
                  Free(last_matched_symbol_name);
                  last_matched_symbol_name = StringDup(prev_matched_symbol_name);
                  last_matched_symbol=prev_matched_symbol;
                }

              CheckAndCastOneArgument ("OR", 1, &(in->args->args[1]), 'T',
                                       function_table, implicit_argument);
              
              if (!(((t_uint32) (unsigned long) in->args->args[1]->data.data) & 1))
                {
                  ret->data.data = (void *) (unsigned long) ((((t_uint32) (unsigned long) in->args->args[0]->data.data) | ((t_uint32) (unsigned long) in->args->args[1]->data.data)) & (~1));
                  ret->args = NULL;
                  AstNodeFree (in);
                  if (last_matched_symbol_name)
                    {
                      Free(last_matched_symbol_name);
                      last_matched_symbol_name = StringDup(prev_matched_symbol_name);
                      last_matched_symbol=prev_matched_symbol;
                      Free(prev_matched_symbol_name);
                    }
                  return ret;
                }       
              else
                {
                  ret->data.data = (void *) (unsigned long) (( (t_uint32) (unsigned long) in->args->args[1]->data.data));
                  ret->args = NULL;
                  AstNodeFree (in);
                  if (last_matched_symbol_name)
                    {
                      Free(last_matched_symbol_name);
                      last_matched_symbol_name = StringDup(prev_matched_symbol_name);
                      last_matched_symbol=prev_matched_symbol;
                      Free(prev_matched_symbol_name);
                    }
                  return ret;
                }
            }
          else
            {
              ret->data.data = (void *) (unsigned long) (( (t_uint32) (unsigned long) in->args->args[0]->data.data));
              ret->args = NULL;
              AstNodeFree (in);
              if (last_matched_symbol_name)
                {
                  Free(last_matched_symbol_name);
                  last_matched_symbol_name = StringDup(prev_matched_symbol_name);
                  last_matched_symbol=prev_matched_symbol;
                  Free(prev_matched_symbol_name);
                }
              return ret;
            }

          LINKER_SCRIPT_FATAL("Unreachable!");
          break;
        }
      case OPER_NOT:
        {
          t_ast_node *ret = Malloc (sizeof (t_ast_node));

          CheckAndCastOneArgument ("NOT", 0, &(in->args->args[0]), 'B',
                                   function_table, implicit_argument);
          ret->type = TYPE_BOOL;
          ret->data.data =
            (void *) (unsigned long) (!((t_bool) in->args->args[0]->data.data));
          ret->args = NULL;
          AstNodeFree (in);
          return ret;

        }
        break;
      case OPER_EQUAL:
      case OPER_NOT_EQUAL:
        {
          t_ast_node *left = in->args->args[0];
          t_ast_node *right = in->args->args[1];

          if (left->type == TYPE_FUNCTION)
	  {
		  int backup_free_mode=current_free_mode;
		  in->args->args[0] = left =
			  AstExecute (left, function_table, implicit_argument, FALSE);
		  current_free_mode=backup_free_mode;
          }

          if (right->type == TYPE_FUNCTION)
          {
		  int backup_free_mode=current_free_mode;
            in->args->args[1] = right =
              AstExecute (right, function_table, implicit_argument,
                          FALSE);
		  current_free_mode=backup_free_mode;
          }

	  if (left->type == TYPE_OPER)
	  {
		  int backup_free_mode=current_free_mode;
		   in->args->args[0] = left =
			   AstExecute (left, function_table, implicit_argument, FALSE);
		  current_free_mode=backup_free_mode;
	  }

	  if (right->type == TYPE_OPER)
	  {
		  int backup_free_mode=current_free_mode;
		   in->args->args[1] = right =
			   AstExecute (right, function_table, implicit_argument, FALSE);
		  current_free_mode=backup_free_mode;
	  }

          if (left->type == TYPE_OTHER)
	  {
            t_string orig = left->data.data;

            left->type = TYPE_NUMERIC;
            left->data.data =
              (void *) (unsigned long) StringToUint64 (orig, 0);
            Free (orig);
	  }

          if (right->type == TYPE_OTHER)
          {
            t_string orig = right->data.data;

            right->type = TYPE_NUMERIC;
            right->data.data =
              (void *) (unsigned long) StringToUint64 (orig, 0);
            Free (orig);
          }


          if ((left->type == TYPE_NUMERIC) && (right->type == TYPE_NUMERIC))
          {
            t_ast_node *ret = Malloc (sizeof (t_ast_node));

            ret->type = TYPE_BOOL;
            if (in->data.oper == OPER_EQUAL)
              ret->data.data =
                (void *) (unsigned long) (left->data.data == right->data.data);
            else
              ret->data.data =
                (void *) (unsigned long) (left->data.data != right->data.data);
            ret->args = NULL;
            AstNodeFree (in);

            return ret;

          }
          else if ((left->type == TYPE_ADDRESS) && (right->type == TYPE_ADDRESS))
          {
            t_ast_node *ret = Malloc (sizeof (t_ast_node));

            ret->type = TYPE_BOOL;
            if (in->data.oper == OPER_EQUAL)
              ret->data.data =
                (void *) (unsigned long) AddressIsEq(left->data.addr,right->data.addr);
            else
              ret->data.data =
                (void *) (unsigned long) !AddressIsEq(left->data.addr,right->data.addr);
            ret->args = NULL;
            AstNodeFree (in);

            return ret;
          }
          else
            LINKER_SCRIPT_FATAL("Implement equal operator on non-numeric arguments (type left = %d, type right = %d)", left->type, right->type);
        }
        break;
      default:
        LINKER_SCRIPT_FATAL("Unknown operand %d in linker script\n", in->data.oper);
    }
  }
  else
  {
    LINKER_SCRIPT_FATAL("Unknown type! %d", in->type);
  }
  /* keep the compiler happy */
  return NULL;
}

/*}}}*/
/* Check and cast one {{{ */
void
CheckAndCastOneArgument (t_string name, int tel, t_ast_node ** to_cast,
                         char desc, t_ast_node_table_entry * function_table,
                         void *implictit_arg)
{
  if ((*to_cast == NULL) && (desc != 'V'))
    LINKER_SCRIPT_FATAL("Function %s expected a type %c as argument %d, and no argument is given", name, desc, tel);
  else if (*to_cast == NULL)
    return;
  switch ((*to_cast)->type)
  {
    case TYPE_STRING:
      if (desc != 'S')
        LINKER_SCRIPT_FATAL("Function %s expected a type %c as argument %d, value = type string", name, desc, tel);
      break;
    case TYPE_NUMERIC:
      if (desc == 'A')
      {
        (*to_cast)->type = TYPE_ADDRESS;
        (*to_cast)->data.addr = AddressNewForObject(parser_object,(t_uint64) (unsigned long) (*to_cast)->data.data);
      }
      else if (desc != 'N')
        LINKER_SCRIPT_FATAL("Function %s expected a type %c as argument %d", name, desc, tel);

      break;
    case TYPE_OTHER:
      if (desc == 'N')
      {
        t_string orig = (*to_cast)->data.data;

        (*to_cast)->type = TYPE_NUMERIC;
        (*to_cast)->data.data =
          (void *) (unsigned long) StringToUint64 (orig, 0);
        Free (orig);
      }
      else if (desc == 'T')
      {
        t_string orig = (*to_cast)->data.data;

        (*to_cast)->type = TYPE_TRIGGER;
        (*to_cast)->data.data =
          (void *) (unsigned long) StringToUint64 (orig, 0);

        if (((t_uint32) (unsigned long) ((*to_cast)->data.data))>2)  
          (*to_cast)->data.data = (void *) 1;
        Free (orig);
      }
      else if (desc == 'A')
      {
        t_string orig = (*to_cast)->data.data;

        (*to_cast)->type = TYPE_ADDRESS;
        (*to_cast)->data.addr = AddressNewForObject(parser_object,StringToUint64 (orig, 0));
        Free (orig);
      }
      else if (desc == 'O')
      {
        break;
      }
      else if ((desc == 'S')
               && (StringToUint64 ((*to_cast)->data.data, 0) == 0))
      {
        t_string orig = (*to_cast)->data.data;

        (*to_cast)->type = TYPE_STRING;
        (*to_cast)->data.data = 0;
        Free (orig);
      }
      else
        LINKER_SCRIPT_FATAL("Function %s expected a type %c as argument %d, value = other", name, desc, tel);
      break;
    case TYPE_VOID:
      if (desc != 'V')
      {
        LINKER_SCRIPT_FATAL("Function %s expected type %c as argument %d", name, desc, tel);
      }
      break;
    case TYPE_TRIGGER:
      if (desc == 'T')
      {
        break;
      }
      else if (desc == 'B')
      {
        (*to_cast)->type = TYPE_BOOL;
        if (((int) (long) (*to_cast)->data.data) >= 2)
        {
          WARNING(("Cast from type trigger to type bool casts away repeating triggers"));
          (*to_cast)->data.data = (void *) (((unsigned long) (*to_cast)->data.data) & (~2));
        }
      }
      else
      {
        LINKER_SCRIPT_FATAL("Function %s expected a type %c as argument %d, value = type trigger", name, desc, tel);
      }
      break;
    case TYPE_BOOL:
      if (desc == 'B')
      {
        break;
      }
      else if (desc == 'T')
      {
         (*to_cast)->type = TYPE_TRIGGER;
      }
      else
        LINKER_SCRIPT_FATAL("Function %s expected a type %c as argument %d, value = type bool", name, desc, tel);
      break;
    case TYPE_OPER:
      {
        t_ast_node *n = AstExecute ((*to_cast), function_table, implictit_arg,
                                    current_free_mode);

        CheckAndCastOneArgument (name, tel, &n, desc, function_table,
                                 implictit_arg);
        (*to_cast) = n;
      }
      break;
    case TYPE_FUNCTION:
      {
        t_ast_node *n = AstExecute ((*to_cast), function_table, implictit_arg,
                                    current_free_mode);

        CheckAndCastOneArgument (name, tel, &n, desc, function_table,
                                 implictit_arg);
        (*to_cast) = n;
      }
      break;
    case TYPE_ADDRESS:
      if (desc == 'A')
      {
        break;
      }
      else
        LINKER_SCRIPT_FATAL("Function %s expected a type %c as argument %d, value = type address", name, desc, tel);

      break;
    case TYPE_SECTION:
      if (desc == 'E')
      {
        break;
      }
      else
        LINKER_SCRIPT_FATAL("Function %s expected a type %c as argument %d, value = type section", name, desc, tel);
      break;
    case TYPE_SYMBOL:
      if (desc == 'Y')
        break;
      LINKER_SCRIPT_FATAL("SYMBOL");
      break;
    default:
      LINKER_SCRIPT_FATAL("Implement type %d\n", (*to_cast)->type);
  }
}

/*}}} */
/* Check And Cast All {{{ */
static void
CheckAndCastArguments (t_ast_node * in, t_uint32 nargs, t_string arg_desc,
                       t_ast_node_table_entry * function_table,
                       void *implictit_arg)
{
  t_ast_successors *args = in->args;
  t_string name = in->data.name;

  if ((nargs == 0) && (args != NULL) && (args->nargs != 0))
  {
    LINKER_SCRIPT_FATAL("Function %s expects no arguments, but %d arguments are given", name, args->nargs);
  }
  else if (nargs != 0)
  {
    if ((args == NULL) || (args->nargs == 0))
      LINKER_SCRIPT_FATAL("Function %s expects %d arguments, but no arguments are given", name, nargs);
    else if (args->nargs != nargs)
      LINKER_SCRIPT_FATAL("Function %s expects %d arguments, but %d arguments are given", name, nargs, args->nargs);
    else
    {
      t_uint32 tel;

      /* The number of arguments is right, now check and cast the arguments */
      for (tel = 0; tel < args->nargs; tel++)
      {
        CheckAndCastOneArgument (name, tel, &(args->args[tel]),
                                 arg_desc[tel], function_table,
                                 implictit_arg);
      }

    }
  }
}

/* }}} */
/* }}} */

/* Linker Rule Execution {{{ */
void
LinkerRuleExecute (t_linker_rule * rule, t_string name)
{
  t_linker_rule *used_rule = LinkerRuleDup (rule);
  t_ast_node *trigger = NULL;
  t_ast_node *ret;
  t_bool rerun = FALSE;

  current_rule_name = name;

  do
  {
    if (rerun)
    {
      AstNodeFree (trigger);
      Free (used_rule);
      used_rule = LinkerRuleDup (rule);
    }

    /* In case no trigger is provided, we assume the trigger is ALWAYS */
    VERBOSE(1,("EVAL %s",current_rule_name));
    if (used_rule->trigger)
      trigger = AstExecute (used_rule->trigger, FunctionTable, NULL, TRUE);
    else 
      trigger = NULL;

    /* Execute when we have no trigger or if the trigger evaluated to TRUE */
    if ((!trigger) || (((int) (long) trigger->data.data) & 1))
    {
      VERBOSE(1, ("EXEC %s", current_rule_name));
      ret =
        AstExecute (used_rule->action, FunctionTable, used_rule, TRUE);
      AstNodeFree (ret);
      if (used_rule->section)
        LINKER_SCRIPT_FATAL("When executing the linker rule %s the section argument was not used!",name);
      if (used_rule->address)
        LINKER_SCRIPT_FATAL("When executing the linker rule %s the address argument was not used!",name);
      if (used_rule->symbol)
        LINKER_SCRIPT_FATAL("When executing the linker rule %s the symbol argument was not used!",name);
    }
    else
    {
      AstNodeFree (used_rule->action);
      if (used_rule->section)
        AstNodeFree (used_rule->section);
      if (used_rule->address)
        AstNodeFree (used_rule->address);
      if (used_rule->symbol)
        AstNodeFree (used_rule->symbol);
    }
    rerun = TRUE;
  }
  while ((trigger) && (((int) (long) trigger->data.data) & 2));

  trigger_pattern_symbol = NULL;
  if (trigger) AstNodeFree (trigger);
  Free (name);
  Free (used_rule);

  AstNodeFree (rule->action);
  
  if (rule->trigger)
    AstNodeFree (rule->trigger);
  if (rule->section)
    AstNodeFree (rule->section);
  if (rule->address)
    AstNodeFree (rule->address);
  if (rule->symbol)
    AstNodeFree (rule->symbol);

  Free (rule);
}

/*}}}*/

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
