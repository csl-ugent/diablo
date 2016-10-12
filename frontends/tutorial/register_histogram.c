#include <diabloflowgraph.h>

t_uint32  * registers;

void ObjectRewrite(t_string name,int (*func)(t_cfg *),t_string oname)
{
  t_object *obj;
  t_cfg *cfg;
  /* Restore a dumped program, it will be loaded by ObjectGet */
  obj = LinkEmulate (name);

  /* 1. Disassemble */

  ObjectDisassemble (obj);
  /* 2.  Create the flowgraph */
  ObjectFlowgraph (obj, NULL, NULL);
  cfg = OBJECT_CFG(obj);

  func(cfg);

  ObjectDeflowgraph (obj);

  /* rebuild the layout of the data sections
   * so that every subsection sits at it's new address */
  ObjectRebuildSectionsFromSubsections (obj);

  ObjectAssemble (obj);

  ObjectWrite (obj, oname);

#ifdef DIABLOSUPPORT_HAVE_STAT
  /* make the file executable */
  chmod (oname,
	 S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
	 S_IXOTH);
#endif

}

t_int32
RegisterHistogram(t_cfg * cfg)
  /* Transform cfg, return 0 if success, <0, or FATAL if error  */
{
  t_bbl * bbl;
  t_ins * ins;
  t_i386_operand operand;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    BBL_FOREACH_INS(bbl,ins)
    {
      I386_INS_FOREACH_OP(ins, operand)
      {
	if(I386_OP_BASE(operand)!= I386_REG_NONE)
	  registers[I386_OP_BASE(operand)]++;
	if(I386_OP_INDEX(operand)!= I386_REG_NONE)
	  registers[I386_OP_INDEX(operand)]++;
      }
    }
  }
  
  {
    t_uint32 i = 0;
    t_uint32 max = i386_description.num_int_regs;
    for(i=0;i<max;i++)
      VERBOSE(0,("%s %ud",i386_description.register_names[i],registers[i]));
  }
}

int
main(int argc, char ** argv)
{
  t_string objectname=NULL;

  registers = Calloc(i386_description.num_int_regs, sizeof(t_uint32));
  	
  DiabloFlowgraphInit(argc,argv); // Inits Object, Support, all backends,...

  ObjectRewrite(reachable_options.input_name,RegisterHistogram,reachable_options.output_name);

  DiabloFlowgraphFini();
}
