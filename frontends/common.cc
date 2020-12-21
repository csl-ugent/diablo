#include "common.h"

#include <fstream>

using namespace std;

void DumpBasicBlocks(t_cfg *cfg, t_const_string filename)
{
  ofstream outfile(filename);
  ASSERT(outfile.is_open(), ("could not open output file [%s]", filename));

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (BBL_IS_HELL(bbl))
      continue;

    if (!BBL_INS_LAST(bbl))
      continue;

    t_address begin = BBL_OLD_ADDRESS(bbl);
    t_address end = INS_OLD_ADDRESS(BBL_INS_LAST(bbl));

    t_const_string fun_name = "-";
    if (BBL_FUNCTION(bbl))
      fun_name = FUNCTION_NAME(BBL_FUNCTION(bbl));

    auto str = StringIo("@G @G %s", begin, end, fun_name);
    outfile << str << endl;
    Free(str);
  }

  outfile.close();
}

/* Create an array of (actually immutable) t_const_string's from a vector<string> */
t_const_string* stringVectorToConstStringArray(const std::vector<std::string> &v)
{
  if (!v.empty())
  {
    t_const_string* cc = new t_const_string[v.size()+1];
    t_uint32 idx = 0;

    for (const std::string& s : v)
    {
      cc[idx] = s.c_str();
      idx++;
    }

    cc[idx] = NULL;

    return cc;
  }

  return NULL;
}

void UniqueFunctionNames(t_cfg *cfg)
{
  set<string> function_names;

  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun)
  {
    if (!FUNCTION_NAME(fun))
      continue;

    if (function_names.find(FUNCTION_NAME(fun)) != function_names.end())
    {
      /* we need to rename this function */
      t_string new_name = StringConcat2(FUNCTION_NAME(fun), "A");
      auto pos = strlen(new_name)-1;
      for (int x = 65; x <= 90; x++)
      {
        new_name[pos] = (char)x;
        if (function_names.find(new_name) == function_names.end())
        {
          /* set the new name */
          FUNCTION_SET_NAME(fun, new_name);
          break;
        }
      }
    }

    function_names.insert(FUNCTION_NAME(fun));
  }
}

void
PrintFullCommandline(int argc, char** argv)
{
  for (int i = 0; i < argc; i++)
    printf("%s ", argv[i]);
  printf("\n");
}

void EnhancedLivenessDirectoryBroker(t_string *s) {
  *s = global_options.liveness_path;
}
