/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "mechanisms.h"

/* C standard headers */
#include <signal.h>
#include <string.h>
#include <stdlib.h>

/* Linux headers */
#include <pthread.h>

pthread_mutex_t first_mutex;
pthread_mutex_t second_mutex;

static uintptr_t tmp_value1 = 0x0f0f0f0f;
static uintptr_t tmp_value2 = 0x0fffffff;
static uintptr_t tmp_value3 = 0x0fff0fff;

typedef struct {
  uintptr_t * int_value_ptr;
  pthread_mutex_t * mutex_ptr;
} reaction_struct;

static reaction_struct x = {&tmp_value1,&first_mutex};
static reaction_struct y = {&tmp_value1,&second_mutex};
static reaction_struct z = {&tmp_value1,&second_mutex};

static reaction_struct *x_ptr = &x;
static reaction_struct *y_ptr = &y;
static reaction_struct *z_ptr = &z;

INIT_REACTION()
{
  pthread_mutex_init(&first_mutex, NULL);
  pthread_mutex_init(&second_mutex, NULL);
}

START_DEGRADATION(original)
{
  y_ptr->int_value_ptr = &tmp_value2;
  z_ptr->int_value_ptr = &tmp_value3;
}

MECHANISM(F)
{
  uintptr_t ret;

  tmp_value1 = first;
  tmp_value2 = first ^ tmp_value2;

  if (first & 0x1)
    ret = *(y_ptr->int_value_ptr);
  else
    ret = *(z_ptr->int_value_ptr);

  return ret;
}

MECHANISM(G)
{
  uintptr_t ret;

  tmp_value1 = first;
  tmp_value2 = first+tmp_value2;

  if (first & 0x3)
    ret = *(y_ptr->int_value_ptr);
  else
    ret = *(z_ptr->int_value_ptr);

  return ret;
}

/*int funky_array[20] = {45,103,56,4203,2386,246,7008,-1000,-4,347862,-574,-263,100,4635,24586,2835,2956};


static int cmpfunc (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

int DIABLO_REACTION_G(int x)
{
  printf("1 %p\n",y_ptr->mutex_ptr);
  pthread_mutex_lock(y_ptr->mutex_ptr);
  funky_array[10]=x;
  if (x == 0x1)
    qsort(funky_array,20,sizeof(int),cmpfunc);
  else
    qsort(funky_array,19,sizeof(int),cmpfunc);
  printf("2 %p\n",z_ptr->mutex_ptr);
  pthread_mutex_unlock(z_ptr->mutex_ptr);
  if (funky_array[0]<0)
    return x;
  else
    return -x;
}*/
