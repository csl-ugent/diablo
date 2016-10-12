/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* Managed Class Typedefs {{{ */
#ifndef DIABLOSUPPORT_MANAGED_CLASS_TYPEDEFS
#define DIABLOSUPPORT_MANAGED_CLASS_TYPEDEFS
#ifndef DIABLOSUPPORT_INTERNAL
#include <Judy.h>

#define DYNAMIC_MEMBER(totype,mantype,Info,type,name,NAME,Name,ITERATOR,Constructor,Destructor,Duplicator)  \
static void Constructor##Wrapper(void * x,void * y) {                                                       \
  void * z;                                                                                                 \
  JLI(z,Info.array,(Word_t) x);                                                                             \
  Constructor((_concat_(t_,totype) *) x, (type *) z);                                                       \
}                                                                                                           \
static void Duplicator##Wrapper(void * x,void * y) {                                                        \
  void * z;                                                                                                 \
  JLI(z,Info.array,(Word_t) x);                                                                             \
  Duplicator((_concat_(t_,totype) *) x, (type *) z);                                                        \
}                                                                                                           \
static void Destructor##Wrapper(void * x,void * y) {                                                        \
  int ret;                                                                                                  \
  void * z;                                                                                                 \
  JLG(z,Info.array,(Word_t) x);                                                                             \
  Destructor((_concat_(t_,totype) *) x, (type *) z);                                                        \
  JLD(ret,Info.array,(Word_t) x);                                                                           \
}                                                                                                           \
static void _concat3_(_concat_(totype,_function_prefix),Init,Name) (mantype man) {                          \
  int init;                                                                                                 \
  J1T(init, Info.init, (Word_t)man);                                                                        \
  if (!init) {                                                                                              \
    _concat_(t_,totype) * to;                                                                               \
    ITERATOR(man,to) {                                                                                      \
      Constructor##Wrapper(to,NULL);                                                                        \
    }                                                                                                       \
    /* Callbacks with a NEGATIVE priority (3rd argument) are executed BEFORE the operation has been done.   \
     * Callbacks with a POSITIVE priority (3rd argument) are executed AFTER  the operation has been done.   \
     * The callbacks are kept in a linked list, of which the order of the callbacks therein is determined   \
     * by the priority value. The callbacks are executed one after another, starting from the head of the   \
     * linked list. Callbacks with the smallest priority (non-absolute) are executed first, callbacks with  \
     * the biggest priority are executed last.                                                              \
     *                                                                                                      \
     * For more information on the linked list implementation, see the function "ManagerCallbackInstall"    \
     * in diablosupport_managed_class.c. */                                                                 \
    _concat_(_concat_(totype,_function_prefix),CallbackInstall)(man,CB_NEW,10,Constructor##Wrapper,NULL);   \
    _concat_(_concat_(totype,_function_prefix),CallbackInstall)(man,CB_FREE,-10,Destructor##Wrapper,NULL);  \
    _concat_(_concat_(totype,_function_prefix),CallbackInstall)(man,CB_DUP,10,Duplicator##Wrapper,NULL);    \
    J1S (init, Info.init, (Word_t)man);                                                                     \
  }                                                                                                         \
}                                                                                                           \
static void _concat3_(_concat_(totype,_function_prefix),Fini,Name) (mantype man) {                          \
  int init;                                                                                                 \
  J1T(init, Info.init, (Word_t)man);                                                                        \
  if (init) {                                                                                               \
    _concat_(t_,totype) * to;                                                                               \
    ITERATOR(man,to) {                                                                                      \
      Destructor##Wrapper(to,NULL);                                                                         \
    }                                                                                                       \
    _concat_(_concat_(totype,_function_prefix),CallbackUninstall)(man,CB_NEW,10,Constructor##Wrapper,NULL); \
    _concat_(_concat_(totype,_function_prefix),CallbackUninstall)(man,CB_FREE,-10,Destructor##Wrapper,NULL);\
    _concat_(_concat_(totype,_function_prefix),CallbackUninstall)(man,CB_DUP,10,Duplicator##Wrapper,NULL);  \
    J1U (init, Info.init, (Word_t)man);                                                                     \
  }                                                                                                         \
}                                                                                                           \
static type _concat3_(_concat_(totype,_field_select_prefix),_,NAME)(const _concat_(t_,totype) * x) {        \
  void * z;                                                                                                 \
  ASSERT(x,("Passed NULL to dynamic getter"));                                                              \
  JLG(z,Info.array,(Word_t) x);                                                                             \
  if (!z)                                                                                                   \
    FATAL(("Field not allocated.... Did you call %sInit%s?\n",#totype,#Name));                              \
  return *((type *) z);                                                                                     \
}                                                                                                           \
static void _concat3_(_concat_(totype,_field_select_prefix),_SET_,NAME)(_concat_(t_,totype) * x, type y) {  \
  void * z;                                                                                                 \
  ASSERT(x,("Passed NULL to dynamic setter")); JLG(z,Info.array,(Word_t) x);                                \
  if (!z)                                                                                                   \
    FATAL(("Field not allocated.... Did you call %sInit%s?\n",#totype,#Name));                              \
  *((type *) z)=y;                                                                                          \
}
#endif

typedef void (*t_manager_callback_function) (void *, void *);
typedef struct _t_manager_callback t_manager_callback;
typedef struct _t_manager t_manager;

typedef struct _t_dynamic_member_info t_dynamic_member_info;

typedef enum
{
  CB_NEW,
  CB_FREE,
  CB_DUP
} t_manager_callback_type;
#endif /* }}} Typedefs */
/* Managed Class Types {{{ */
#ifndef DIABLOSUPPORT_MANAGED_CLASS_TYPES
#define DIABLOSUPPORT_MANAGED_CLASS_TYPES
struct _t_manager_callback
{
  t_manager_callback_type type;
  t_int32 prior;
  t_manager_callback_function cb_func;
  void *cb_user_data;
  struct _t_manager_callback *next;
};

struct _t_manager
{
  t_bool init;
  t_manager_callback *new_;
  t_manager_callback *free;
  t_manager_callback *dup;
};

struct _t_dynamic_member_info
{
  /* the actual judy array with the dynamic member values */
  void *array;
  /* a judy array keeping track of which managers have been initialized */
  void *init;
};

#define null_info {NULL, NULL} /* Check with Jonas: actual bug?? TODO BUG */

#endif /* }}} Types */
#include <diablosupport.h>
#ifdef DIABLOSUPPORT_FUNCTIONS
/* Managed Class Functions {{{ */
#ifndef DIABLOSUPPORT_MANAGED_CLASS_FUNCTIONS
#define DIABLOSUPPORT_MANAGED_CLASS_FUNCTIONS
void ManagerCallbackInstall (t_manager *, t_manager_callback_type, t_int32, t_manager_callback_function, void *);
#endif /* }}} Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
