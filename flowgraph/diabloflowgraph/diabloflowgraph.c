/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#define GENERATE_CLASS_CODE
#include <diabloflowgraph.h>

//#define PRINT_INS_POINTER
#define PRINT_BBL_POINTER
#define PRINT_BBL_REGSETS
//#define PRINT_EDGE_POINTER

/* Instructions {{{ */
t_string
IoModifierIns (t_const_string modifiers, va_list * ap)
{
  t_string ret;
  t_bool print_address = TRUE;
  char buffer[2000];
  t_string_array *array;
  t_ins *ins = va_arg (*ap, t_ins *);
  t_bool print_phase = FALSE;
  t_bool print_phase_extended = FALSE;
  t_bool print_debug_info = FALSE;
  t_bool print_transformation = FALSE;
  t_bool print_flags = FALSE;

  array = StringArrayNew ();

  for (; *modifiers != 0; modifiers++)
  {
    if ((*modifiers) == 'a') print_address = FALSE;
    if ((*modifiers) == 'p') print_phase = TRUE;
    if ((*modifiers) == 'x') print_phase_extended = TRUE;
    if ((*modifiers) == 'g') print_debug_info = TRUE;
    if ((*modifiers) == 't') print_transformation = TRUE;
    if ((*modifiers) == 'f') print_flags = TRUE;
  }

  if (print_debug_info && diabloobject_options.read_debug_info)
    {
      char * line_info;

      if (INS_SRC_LINE(ins)!=0)
        line_info = StringIo("(l. %5d) ",INS_SRC_LINE(ins));
      else
        line_info = StringDup("         ");
      StringArrayAppendString(array, line_info);
    }


  if (print_address)
  {
    if (!AddressIsEq (INS_OLD_ADDRESS(ins), INS_CADDRESS(ins)))
      StringArrayAppendString (array, StringDup ("New "));

    StringArrayAppendString (array, StringIo ("@G", INS_CADDRESS(ins)));

    if (!AddressIsEq (INS_OLD_ADDRESS(ins), INS_CADDRESS(ins)))
    {
      StringArrayAppendString (array, StringDup (" Old "));
      StringArrayAppendString (array, StringIo ("@G", INS_OLD_ADDRESS(ins)));
    }
#ifdef PRINT_INS_POINTER
    StringArrayAppendString (array, StringIo (" [%p]: ", ins));
#endif
  }

  if (INS_BBL(ins))
    CFG_DESCRIPTION(BBL_CFG(INS_BBL(ins)))->InsPrint (ins, buffer);
  else
  {
    const t_architecture_description *desc = ObjectGetArchitectureDescription (INS_OBJECT(ins));

    desc->InsPrint (ins, buffer);
  }

  StringArrayAppendString (array, StringDup (buffer));
  DiabloBrokerCall("IoModifierInsAF", ins, array);

  if (print_phase){
    if (print_phase_extended)
      StringArrayAppendString (array, StringIo ("(phase: %s)", GetDiabloPhaseName(INS_PHASE(ins))));
    else
      StringArrayAppendString (array, StringIo ("(phase: %d)", INS_PHASE(ins)));
  }

  if (print_transformation){
    StringArrayAppendString(array, StringIo("(transformation: 0x%016" PRIx64 ")", INS_TRANSFORMATION_ID(ins)));
  }

  if (print_flags){
    StringArrayAppendString(array, StringIo("(flags: 0x%x)", INS_ATTRIB(ins)));
  }

  ret = StringArrayJoin (array, " ");
  StringArrayFree (array);
  return ret;

}

/* }}} */
/* Bbls {{{ */
t_string
IoModifierBbl (t_const_string modifiers, va_list * ap)
{
  t_bbl *bbl = va_arg (*ap, t_bbl *);
  t_string_array *array = StringArrayNew ();
  t_string ret;
  t_bool flag = FALSE;
  t_const_string orig_modifiers = modifiers;
  t_bool print_function = TRUE;
  StringArrayAppendString (array, StringDup ("bbl at"));
#ifdef PRINT_BBL_POINTER
  StringArrayAppendString (array, StringIo (" [%p]", bbl));
#endif
#ifdef PRINT_BBL_REGSETS
  StringArrayAppendString (array, StringIo ("\\l live-before: @X\\l live-after: @X\\l", CPREGSET(BBL_CFG(bbl), BblRegsLiveBefore(bbl)), CPREGSET(BBL_CFG(bbl), BblRegsLiveAfter(bbl))));
#endif

  if (AddressIsEq (BBL_OLD_ADDRESS(bbl), BBL_CADDRESS(bbl)))
  {
    StringArrayAppendString (array, StringIo ("@G", BBL_OLD_ADDRESS(bbl)));
  }
  else
  {
    StringArrayAppendString (array, StringIo ("new: @G old: @G ", BBL_CADDRESS(bbl), BBL_OLD_ADDRESS(bbl)));
  }

  for (; *modifiers != 0; modifiers++)
    {
      if ((*modifiers) == 'x')
        print_function = FALSE;
    }

  modifiers = orig_modifiers;

  if (print_function)
  {
    if (BBL_FUNCTION(bbl))
    {
      StringArrayAppendString (array, StringIo ("(in %s at @G)", FUNCTION_NAME(BBL_FUNCTION(bbl)), BBL_CADDRESS(FUNCTION_BBL_FIRST(BBL_FUNCTION(bbl)))));
    }
    else
    {
      StringArrayAppendString (array, StringDup ("(in no fun)"));
    }
  }

  if (BBL_EXEC_COUNT(bbl))
  {
    StringArrayAppendString (array, StringIo (" exec %d hotness %d", BBL_EXEC_COUNT(bbl), BBL_EXEC_COUNT(bbl)*BBL_NINS(bbl)));
  }

  if (BBL_SEQUENCE_ID(bbl))
  {
    StringArrayAppendString (array, StringIo (" seq %d", BBL_SEQUENCE_ID(bbl)));
  }

  if (print_function)
    StringArrayAppendString (array, StringDup ("\n"));
  else
    StringArrayAppendString (array, StringDup ("\\l"));

  DiabloBrokerCall("IoModifierBbl", bbl, array);
  DiabloBrokerCall("IoModifierAF", bbl, array);
  DiabloBrokerCall("IoModifierBblNonZero", bbl, array);
  DiabloBrokerCall("IoModifierBblTracking", bbl, array);
  DiabloBrokerCall("IoModifierBblMetaAPI", bbl, array);

  for (; *modifiers != 0; modifiers++)
  {
    if ((*modifiers) == 'i')
    {
      t_ins *ins;
      char insformat[8] = {'@','t','I','\n','\0'};
      t_const_string tmp_modif = orig_modifiers;

      for (; *tmp_modif != 0; tmp_modif++)
        if ((*tmp_modif) == 'a') sprintf(insformat,"@atI\n");

      StringArrayAppendString (array, StringDup ("Instructions:\n"));
      BBL_FOREACH_INS(bbl, ins)
      {
        StringArrayAppendString (array, StringIo (insformat, ins));
      }
    }
    else if ((*modifiers) == 'e')
    {
      t_cfg_edge *edge;
      int nr_edges = 0;

      StringArrayAppendString (array, StringDup ("Edges in:\n"));

      BBL_FOREACH_PRED_EDGE(bbl, edge)
        nr_edges++;

      if (nr_edges < 1000)
        BBL_FOREACH_PRED_EDGE(bbl, edge)
        {
          StringArrayAppendString (array, StringIo ("@E\n", edge));
        }
      else
      {
        StringArrayAppendString (array, StringDup ("too many pred edges\n"));
      }

      StringArrayAppendString (array, StringDup ("Edges out:\n"));

      nr_edges = 0;
      BBL_FOREACH_SUCC_EDGE(bbl, edge)
        nr_edges++;

      if (nr_edges < 100)
        BBL_FOREACH_SUCC_EDGE(bbl, edge)
        {
          StringArrayAppendString (array, StringIo ("@E\n", edge));
        }
      else
      {
        StringArrayAppendString (array, StringDup ("too many succ edges\n"));
      }

    }
    else if ((*modifiers) == '-')
      flag = TRUE;
  }
  if (flag)
  {
    StringArrayAppendString (array, StringDup ("-----------------------------------\n"));
  }
  ret = StringArrayJoin (array, " ");
  StringArrayFree (array);
  return ret;
}

/* }}} */
/* CfgEdge {{{ */
t_string
IoModifierCfgEdge (t_const_string modifiers, va_list * ap)
{
  t_cfg_edge *edge = va_arg (*ap, t_cfg_edge *);
  t_string_array *array = StringArrayNew ();
  t_string ret;
  t_address addr1 = BBL_OLD_ADDRESS(CFG_EDGE_HEAD(edge));
  t_address addr2 = BBL_OLD_ADDRESS(CFG_EDGE_TAIL(edge));

  StringArrayAppendString (array, StringIo ("Old: @G New: @G", addr1, BBL_CADDRESS(CFG_EDGE_HEAD(edge))));
  if (CfgEdgeIsFake(edge))
    StringArrayAppendString(array, StringDup("FAKE"));
#ifdef PRINT_EDGE_POINTER
  StringArrayAppendString (array, StringIo (" [%p]", CFG_EDGE_HEAD(edge)));
#endif
  if (BBL_FUNCTION(CFG_EDGE_HEAD(edge)))
  {
    StringArrayAppendString (array, StringIo (" (fun %s at @G)", FUNCTION_NAME(BBL_FUNCTION(CFG_EDGE_HEAD(edge))), BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(BBL_FUNCTION(CFG_EDGE_HEAD(edge))))));
  }
  StringArrayAppendString (array, StringDup (" -> "));
  StringArrayAppendString (array, StringIo ("Old: @G New: @G", addr2, BBL_CADDRESS(CFG_EDGE_TAIL(edge))));
#ifdef PRINT_EDGE_POINTER
  StringArrayAppendString (array, StringIo (" [%p]", CFG_EDGE_TAIL(edge)));
#endif
  if (BBL_FUNCTION(CFG_EDGE_TAIL(edge)))
  {
    StringArrayAppendString (array, StringIo (" (fun %s at @G)", FUNCTION_NAME(BBL_FUNCTION(CFG_EDGE_TAIL(edge))), BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(BBL_FUNCTION(CFG_EDGE_TAIL(edge))))));
  }
  switch (CFG_EDGE_CAT(edge))
  {
    case ET_UNKNOWN:
      StringArrayAppendString (array, StringDup (" type unknown"));
      break;
    case ET_FALLTHROUGH:
      StringArrayAppendString (array, StringDup (" type fallthrough"));
      break;
    case ET_CALL:
      StringArrayAppendString (array, StringDup (" type call"));
      break;
    case ET_RETURN:
      StringArrayAppendString (array, StringDup (" type return"));
      break;
    case ET_JUMP:
      StringArrayAppendString (array, StringDup (" type jump"));
      break;
    case ET_SWI:
      StringArrayAppendString (array, StringDup (" type swi"));
      break;
    case ET_IPUNKNOWN:
      StringArrayAppendString (array, StringDup (" type ipunknown"));
      break;
    case ET_IPFALLTHRU:
      StringArrayAppendString (array, StringDup (" type ipfallthrough"));
      break;
    case ET_IPJUMP:
      StringArrayAppendString (array, StringDup (" type ipjump"));
      break;
    case ET_COMPENSATING:
      StringArrayAppendString (array, StringDup (" type compensating"));
      break;
    case ET_SWITCH:
      StringArrayAppendString (array, StringDup (" type switch"));
      StringArrayAppendString (array, StringIo (" %d", CFG_EDGE_SWITCHVALUE(edge)));
      break;
    case ET_IPSWITCH:
      StringArrayAppendString (array, StringDup (" type ipswitch"));
      StringArrayAppendString (array, StringIo (" %d", CFG_EDGE_SWITCHVALUE(edge)));
      break;
    default:
      StringArrayAppendString (array, StringIo (" type STRANGE %d!", CFG_EDGE_CAT(edge)));
  }

  if (CFG_EDGE_EXEC_COUNT(edge) != 0)
    StringArrayAppendString (array, StringIo (" and exec %lld", CFG_EDGE_EXEC_COUNT(edge)));

  DiabloBrokerCall("IoModifierEdgeAF", edge, array);

  ret = StringArrayJoin (array, "");
  StringArrayFree (array);
  return ret;
}

/* }}} */
/* Regset {{{ */
t_string
IoModifierRegset (t_const_string modifiers, va_list * ap)
{
  t_architecture_description *desc = va_arg (*ap, t_architecture_description *);
  t_regset regs = va_arg (*ap, t_regset);
  t_string_array *array = StringArrayNew ();
  t_bool flag = 0;
  t_reg reg;
  t_regset allregs = desc->all_registers;
  t_string ret;
  int num = 0;

  RegsetSetIntersect(regs,allregs);

  /* architecture-specific printer */
  t_bool arch_specific = false;
  DiabloBrokerCall("ArchitectureSpecificRegsetPrinter", &regs, array, &arch_specific);

  if (!arch_specific) {
    REGSET_FOREACH_REG(regs,reg)
    {
      if (flag)
      {
        StringArrayAppendString (array, StringIo (", %s", desc->register_names[reg]));
        num+=strlen(desc->register_names[reg])+2;
      }
      else
      {
        StringArrayAppendString (array, StringIo ("%s", desc->register_names[reg]));
        num+=strlen(desc->register_names[reg])+2;
      }
      flag = 1;
      if(num>100)
      {
        num=0;
        flag = 0;
        StringArrayAppendString (array, StringIo ("\\l  "));
      }
    }
  }

  ret = StringArrayJoin (array, "");
  StringArrayFree (array);
  return ret;
}
/* }}} */

/* {{{ Function IoModifier */
t_string
IoModifierFunction (t_const_string modifiers, va_list * ap)
{
  t_function *fun  = va_arg (*ap, t_function *);

  if (FUNCTION_NAME(fun))
    return StringDup(FUNCTION_NAME(fun));
  else
    return StringDup("No name");
}
/* }}} */

static t_bool kernel_mode = FALSE;

void DiabloFlowgraphSetKernelMode()
{
  kernel_mode = TRUE;
}

void DiabloFlowgraphUnsetKernelMode()
{
  kernel_mode = FALSE;
}

t_bool DiabloFlowgraphInKernelMode()
{
  return kernel_mode;
}

static int flowgraph_module_usecount = 0;

void
DiabloFlowgraphCmdlineVersion ()
{
  printf ("DiabloFlowgraph version %s\n", DIABLOFLOWGRAPH_VERSION);
}

void
DiabloFlowgraphInit (int argc, char **argv)
{
  if (!flowgraph_module_usecount)
  {
    DiabloObjectInit (argc, argv);
    DiabloFlowgraphCmdlineInit ();
    OptionParseCommandLine (diabloflowgraph_option_list, argc, argv, FALSE);
    OptionGetEnvironment (diabloflowgraph_option_list);
    DiabloFlowgraphCmdlineVerify ();
    OptionDefaults (diabloflowgraph_option_list);
    IoModifierAdd ('@', 'I', "a", IoModifierIns);
    IoModifierAdd ('@', 'B', "ie-a", IoModifierBbl);
    IoModifierAdd ('@', 'E', "", IoModifierCfgEdge);
    IoModifierAdd ('@', 'X', "", IoModifierRegset);
    IoModifierAdd ('@', 'F', "", IoModifierFunction);

    DiabloBrokerCallInstall("DeflowgraphedModus", "t_address in, t_reloc * rel, t_address * out,", DeflowgraphedModus, TRUE);

    DiabloFlowgraphCppInit (argc, argv);
  }
  flowgraph_module_usecount++;
}

void
DiabloFlowgraphFini ()
{
  flowgraph_module_usecount--;
  if (!flowgraph_module_usecount)
  {
    DiabloFlowgraphCppFini ();
    DiabloFlowgraphCmdlineFini ();
    DiabloObjectFini ();
  }
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
