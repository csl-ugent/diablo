/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "mechanisms.h"

/* C standard headers */
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

/* Linux headers */
#include <pthread.h>

typedef struct {
  pthread_mutex_t mutex;
  void* ptr;
} reaction_struct;

static reaction_struct x;
static reaction_struct y;
static bool even = true;
static bool mutex_even = false;

INIT_REACTION()
{
  pthread_mutex_init(&x.mutex, NULL);
  pthread_mutex_init(&y.mutex, NULL);
  pthread_mutex_lock(&y.mutex);
  x.ptr = malloc(27);
}

START_DEGRADATION(complex)
{
  even = !even;
  mutex_even = !mutex_even;
}

/* Corrupt malloc through a double free */
MECHANISM(double_free)
{
  if (even)
  {
    free(x.ptr);
    y.ptr = malloc(27);
  }
  else
  {
    free(y.ptr);
    x.ptr = malloc(27);
  }

  even = !even;

  return first;
}

/* Lock some mutex, hopefully leading to a deadlock in the future */
MECHANISM(mutex_lock)
{
  if (mutex_even)
  {
    pthread_mutex_unlock(&x.mutex);
    pthread_mutex_lock(&y.mutex);
  }
  else
  {
    pthread_mutex_unlock(&y.mutex);
    pthread_mutex_lock(&x.mutex);
  }

  mutex_even = !mutex_even;

  return first;
}

#if 0
/* Shuffle a datastructure */
MECHANISM(shuffle_structure, size_t len, size_t size)
{
  char tmp[size];

  size_t iii = 0;
  for (; iii < (len / 2); iii++)
  {
    memcpy(tmp, (char*)first + iii * size, size);
    memcpy((char*)first + iii * size, (char*)first + (iii + 1) * size, size);
    memcpy((char*)first + (iii + 1) * size, tmp, size);
  }
}

/* These mechanisms contain assembly */
#ifdef __arm__
/* Corrupt some - callee-saved - registers by adding/subtracting values */
INLINE_MECHANISM(corrupt_registers)
{
  __asm("add r4, r4, #64");
  __asm("add r7, r7, #4");
  __asm("sub r6, r6, #20");
  __asm("sub r9, r9, #80");
  __asm("add r8, r8, #16");
  __asm("add r5, r5, #40");
}

/* Corrupt the stack pointer by adding/subtracting an offset */
INLINE_MECHANISM(corrupt_stack_pointer)
{
  __asm("add sp, sp, #400");
}

/* Shuffle the  callee-saved registers */
INLINE_MECHANISM(shuffle_registers)
{
  __asm("mov r0, r5");
  __asm("mov r1, r7");
  __asm("mov r2, r11");
  __asm("mov r3, r9");
  __asm("mov r5, r10");
  __asm("mov r11, r4");
  __asm("mov r9, r6");
  __asm("mov r7, r8");
  __asm("mov r8, r1");
  __asm("mov r4, r2");
  __asm("mov r6, r3");
  __asm("mov r10, r0");
}

INLINE_MECHANISM(shuffle_stack)
{
  __asm("add sp, sp, #400");
}

/* Throw a SIGRAP signal, invoking the mini-debugger */
INLINE_MECHANISM(throw_signal)
{
  __asm("mov r0, #0");
  __asm("bkpt");
}

/* Simply put all registers - including SP, LR and PC - to zero. This will leave no trace to attackers as
 * to how the program ended up here (unless they were instruction tracing).
 */
INLINE_MECHANISM(zero_registers)
{
  __asm("mov r0, #0");
  __asm("mov r1, #0");
  __asm("mov r2, #0");
  __asm("mov r3, #0");
  __asm("mov r4, #0");
  __asm("mov r5, #0");
  __asm("mov r6, #0");
  __asm("mov r7, #0");
  __asm("mov r8, #0");
  __asm("mov r9, #0");
  __asm("mov r10, #0");
  __asm("mov r11, #0");
  __asm("mov r12, #0");
  __asm("mov r13, #0");
  __asm("mov r14, #0");
  __asm("mov r15, #0");
}
#endif
#endif
