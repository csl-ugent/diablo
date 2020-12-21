/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef ARM_OBFUSCATIONS
#define ARM_OBFUSCATIONS

#define ARM_REG_SP ARM_REG_R13
#define ARM_REG_LR ARM_REG_R14
#define ARM_REG_PC ARM_REG_R15

extern "C" {
#include <diabloarm.h>
#include <diabloanoptarm.h>
}
#include <diabloarm.hpp>

#include <obfuscation/obfuscation_architecture_backend.h>
#include <obfuscation/meta_api/meta_api.h>

#endif /* ARM_OBFUSCATIONS */
