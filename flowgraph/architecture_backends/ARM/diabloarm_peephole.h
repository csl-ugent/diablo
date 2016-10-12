#include <diabloflowgraph.h>

void  ArmPeepholeOptimizations(t_cfg * cfg);
t_uint32  ArmPeepholeCombineAdds(t_cfg * cfg);
t_uint32  ArmPeepholeUselessAdds(t_cfg * cfg);
void SwitchMoves(t_cfg * cfg);
t_uint32 ArmPeepholeCombineIdenticalConditionalIns(t_cfg * cfg);
