#include <diabloarm.hpp>

#include <fstream>

using namespace std;

void CfgConstantDistribution(t_cfg *cfg, t_string base_path) {
  map<t_uint64, int> constants;
  
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    t_arm_ins *ins;
    BBL_FOREACH_ARM_INS(bbl, ins) {
      if (ARM_INS_OPCODE(ins) == ARM_CONSTANT_PRODUCER)
        constants[ARM_INS_IMMEDIATE(ins)]++;
    }
  }
  
  t_string filename = StringConcat2(base_path, ".constants");
  ofstream outfile(filename);
  ASSERT(outfile.is_open(), ("could not open output file [%s]", filename));
  Free(filename);
  
  for (auto pair : constants) {
    t_string str = StringIo("%lld,%d", pair.first, pair.second);
    outfile << str << endl;
    Free(str);
  }
  
  outfile.close();
}

void DiabloArmCppInit(int, char **)
{
  SetArchitectureInfoWrapper(new ARMArchitectureInfoWrapper());
  
  DiabloBrokerCallInstall("CfgConstantDistribution", "t_cfg* cfg, t_string base_path", (void *)CfgConstantDistribution, FALSE);
}

void DiabloArmCppFini()
{
  delete (ARMArchitectureInfoWrapper *)GetArchitectureInfoWrapper();
}
