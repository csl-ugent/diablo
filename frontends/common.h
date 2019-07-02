#ifndef FRONTENDS_COMMON_H
#define FRONTENDS_COMMON_H

#include <fcntl.h>

#include <vector>
#include <string>

#include <diabloflowgraph.hpp>

void DumpBasicBlocks(t_cfg *cfg, t_const_string filename);
t_const_string* stringVectorToConstStringArray(const std::vector<std::string> &v);
void UniqueFunctionNames(t_cfg *cfg);
void PrintFullCommandline(int argc, char** argv);

#endif /* FRONTENDS_COMMON_H */
