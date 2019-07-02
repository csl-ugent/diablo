#include "api.h"
#include <unistd.h>

#include <cassert>
#include <fstream>

using namespace std;

int main(int argc, char** argv)
{
  assert(argc == 3);
  auto mobileout = argv[1];
  auto jsonfile = argv[2];

  string str;
  {
    /* read in the json file */
    ifstream t(jsonfile);

    t.seekg(0, ios::end);
    str.reserve(t.tellg());
    t.seekg(0, ios::beg);

    str.assign((istreambuf_iterator<char>(t)),
                istreambuf_iterator<char>());
  }

  /* communicate with proxy */
  SoftVMProxyInit();
  SoftVMProxySetMobileCodeOutputDir(mobileout);
  SoftVMProxyDiabloPhase2(const_cast<t_string>(str.c_str()), NULL);

  /* close server */
  SoftVMProxyDestroy();

  return 0;
}
