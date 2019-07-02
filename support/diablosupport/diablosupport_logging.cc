/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport.hpp>
#include <limits>
#include <string>
#include <vector>
using namespace std;

#define PHASE_MAX 512

t_uint32 diablo_phase = 0;

t_const_string diablo_phases [PHASE_MAX] = {NULL};

FILE *killed_instruction_log = NULL;

struct TransformationCounter {
  uint64_t id;
  uint64_t next_id;
};
vector<TransformationCounter> tf_list = {TransformationCounter{0, 0}};

string GetTransformationIdString() {
  string tf_id_str = "";
  for (size_t x = 0; x < tf_list.size()-1; x++) {
    if (x > 0) tf_id_str += ".";
    tf_id_str += to_string(tf_list[x].id);
  }
  return tf_id_str;
}

t_string GetTransformationIdCString() {
  return StringDup(GetTransformationIdString().c_str());
}

static bool flowgraphing = true;
static bool deflowgraphing = false;
void SetTransformationIdForDeflowgraph() {
  deflowgraphing = true;
}

uint64_t GetTransformationId() {
  if (flowgraphing)
    return -1;
  if (deflowgraphing)
    return -2;

  /* don't use a union-struct here, because the order of the elements in the struct can be undefined (?) */
  uint64_t x = 0;

  // always present
  ASSERT(tf_list[0].id < numeric_limits<uint32_t>::max(), ("transformation depth 0 max limit exceeded (%d)", tf_list[0].id));
  x |= static_cast<uint64_t>(tf_list[0].id) << 32;

  //
  if (tf_list.size()-1 > 1) {
    ASSERT(tf_list[1].id < numeric_limits<uint16_t>::max(), ("transformation depth 1 max limit exceeded (%d)", tf_list[1].id));
    x |= static_cast<uint16_t>(tf_list[1].id) << 16;
  }
  if (tf_list.size()-1 > 2) {
    ASSERT(tf_list[2].id < numeric_limits<uint8_t>::max(), ("transformation depth 2 max limit exceeded (%d)", tf_list[2].id));
    x |= static_cast<uint8_t>(tf_list[2].id) << 8;
  }
  if (tf_list.size()-1 > 3) {
    ASSERT(tf_list[3].id < numeric_limits<uint8_t>::max(), ("transformation depth 3 max limit exceeded (%d)", tf_list[3].id));
    x |= static_cast<uint8_t>(tf_list[3].id);
  }
  if (tf_list.size()-1 > 4) {
    FATAL(("unsupported nested transformation level %d", tf_list.size()));
  }

  return x;
}

TransformationID GetTransformationNumberFromId(uint64_t id) {
  return static_cast<TransformationID>((id >> 32) & 0xffffffff);
}

void DiabloSupportLoggingFini()
{
  t_uint32 iii;
  for (iii = 0; iii < PHASE_MAX; iii++)
    if(diablo_phases[iii])
      Free(diablo_phases[iii]);

  if (killed_instruction_log)
    fclose(killed_instruction_log);
}

static t_string current_transformation_directory = NULL;
//t_uint64 current_transformation_number = -1;

void StartTransformation() {
  flowgraphing = false;
  deflowgraphing = false;

  tf_list.back().next_id++;
  tf_list.push_back(TransformationCounter{tf_list.back().next_id, 0});

  //current_transformation_number++;

  if (!diablosupport_options.enable_transformation_dumps)
    return;

  /* If the parent directory does not exist we cannot create subdirectories */
  DirMake(diablosupport_options.transformation_log_path, FALSE /* tmp */);

  /* TODO: clean subdirectory */
  current_transformation_directory = StringIo("%s/transformation-%" PRId64, diablosupport_options.transformation_log_path, GetTransformationId());

  DirMake(current_transformation_directory, FALSE);
}

t_const_string GetCurrentTransformationDirectory() {
  return current_transformation_directory;
}

void EndTransformation() {
  tf_list.pop_back();
  tf_list.back().id++;
  tf_list.back().next_id = 0;

  if (!diablosupport_options.enable_transformation_dumps)
    return;

  Free(current_transformation_directory);
  current_transformation_directory = NULL;
}

void OpenKilledInstructionLog(t_string filename) {
  killed_instruction_log = fopen(filename, "w");
}

void LogKilledInstruction(t_address addr) {
  if (!killed_instruction_log)
    return;

  if (AddressIsEq(addr, AddressNew32(0)))
    return;

  FileIo(killed_instruction_log, "@G\n", addr);
}

void LogKilledInstructionBarrier(t_string str) {
  if (!killed_instruction_log)
    return;

  FileIo(killed_instruction_log, "%s\n", str);
}

t_string OutputFilename() {
  ASSERT(DiabloBrokerCallExists("OutputFileName"), ("OutputFilename called but broker call with name OutputFileName does not exist. It should be installed in the frontend."));

  t_string x;
  DiabloBrokerCall("OutputFileName", &x);

  return x;
}

static set<FILE *> opened_logs;
void RegisterOpenedLog(LogFile *lf) {
  opened_logs.insert(lf->fp);
}
void UnregisterOpenedLog(LogFile *lf) {
  opened_logs.erase(lf->fp);
}
set<FILE *> GetOpenedLogFiles() {
  return opened_logs;
}