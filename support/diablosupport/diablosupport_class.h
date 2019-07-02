/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef _MSC_VER
#include "diablosupport_class_win32.h"
#else

#include <stddef.h>

/* Utility defines and types */
#ifndef _define_concat_helper_
typedef void renamed_void;
#define force_cast(type,thing) (*( (type *) &(thing) ))
#define _define_concat_helper_(a,b) a ## b
#define _define_concat3_helper_(a,b,c) a ## b ## c
#define _concat_(a,b) _define_concat_helper_(a,b)
#define _concat3_(a,b,c) _define_concat3_helper_(a,b,c)
#define _t_CLASS _concat_(_t_,CLASS) /* object type */
#define ClassOffsetOf(TYPE, MEMBER) OffsetOf(TYPE, _concat_(_hidden_,MEMBER))
#define OffsetOf(TYPE, MEMBER) offsetof(TYPE, MEMBER)
#define ClassCast(TYPE, INST) (&(INST->_concat_(_hidden_parent_,TYPE)))
#define GetBase(x)   (&(x->_hidden_parent))
#endif

#undef GEN_CODE
#undef GEN_CODE_STATIC
#undef GEN_PROTO

#ifdef CLASS_EXPANDS_IN_HEADER 
  #define GEN_CODE_STATIC
#else
  #ifdef GENERATE_CLASS_CODE
    #define GEN_CODE
  #else
    #define GEN_PROTO
  #endif
#endif

/* DIABLO_CLASS_BEGIN: Start of a class, if you are implementing a new class you
 * need to define the following macros before you can use this macro (and the
 * rest of the macros in this file): 
 * 
 * - define CLASS to the name of the class you wish to define (we'll assume it is my_class
 *   in the explanation of the other macros) 
 * - my_class_field_select_prefix to MY_CLASS (ALL_CAPITAL version of my_class)
 * - my_class_function_prefix to MyClass (CamelBack capitalization of my_class) 
 *
 * So your my_class.class.h file would start with:
 *
 * #include <diablosupport_class.h>
 *
 * #ifndef CLASS 
 * #define CLASS my_class
 * #define my_class_field_select_prefix MY_CLASS
 * #define my_class_function_prefix MyClass
 * #endif
 *
 * The ifndef CLASS is needed in case of inheritance (CLASS is already defined
 * if you are reusing this header file to define the inherited fields of a class
 * that inherites from this class).
 *
 *
 * If you are implementing a managed class (i.e. a class to which you can add
 * dynamic fields) you also need to define 
 * - MANAGER_TYPE to the type of the manager (typically a container: e.g. for
 *   t_edge this would be t_graph)
 * - MANAGER_NAME to the name of the field (MEMBER) that holds a pointer to the
 *   manager (container) 
 * - MANAGER_FIELD to the field inside the manager class that will store dynamic
 *   information about this class.
 */
#undef DIABLO_CLASS_BEGIN

/* DIABLO_CLASS_END: marks the end of a class definition */
#undef DIABLO_CLASS_END

/* MEMBER: field, for which getters and setters need to be made. Takes 3
 * arguments: 
 *
 * - field type
 * - lower case version of the name of the field
 * - upper case version of the name of the field
 *
 * This define will allocate space for the field and generate the appropriate
 * getter and setter functions (for class my_class and field my_field =
 * MEMBER(t_field_type, my_field, MY_FIELD) the functions
 *
 * t_field_type MY_CLASS_MY_FIELD(t_CLASS * class);
 *
 * and
 *
 * void MY_CLASS_SET_MY_FIELD(t_CLASS * class, t_field_type value);
 */

#undef MEMBER

/* MEMBERU: special version of MEMBER, calls CallUpdateMY_FIELD each time the
 * value of the field is changed */
#undef MEMBERU
#undef IMEMBER
#undef CONSTRUCTOR
#undef CONSTRUCTOR1
#undef CONSTRUCTOR2
#undef CONSTRUCTOR3
#undef DESTRUCTOR
#undef DUPLICATOR

/* EXTENDS: should be the first statement following DIABLO_CLASS_BEGIN, and
 * defines the base class of this class (the type from which this class inherits):
 *
 * Example:
 *
 * #ifndef CLASS
 * #define CLASS my_class
 * #define my_class_field_select_prefix MY_CLASS
 * #define my_class_function_prefix MyClass
 * #endif
 *
 * DIABLO_CLASS_BEGIN
 * EXTENDS(t_my_parent_class)
 * MEMBER(t_field_type, my_field, MY_FIELD)
 * DIABLO_CLASS_END
 *
 * This will produce a class t_my_class that has all the members (and functions) from
 * t_my_parent_class (so MY_CLASS_PARENT_CLASS_FIELD and
 * MY_CLASS_SET_PARENT_CLASS_FIELD will exist) as will as the field my_field and
 * getters and setters). Furthermore an object of type t_my_class * can be cast
 * to a t_my_parent_class * and subsequently used as a t_my_parent_class *.
 * */
#undef EXTENDS
/* Function with 0 arguments */
#undef FUNCTION0
/* Function with 1 argument */
#undef FUNCTION1
/* Function with 2 arguments */
#undef FUNCTION2
#undef FUNCTION3
#undef FUNCTION4
#undef FUNCTION5
#undef FUNCTION6
#undef FUNCTION7
#undef PFUNCTION0
#undef PFUNCTION1
#undef PFUNCTION2
#undef PFUNCTION3
#undef PFUNCTION4
#undef PFUNCTION5
#undef PFUNCTION6
#undef PFUNCTION7
#undef renamed_t_CLASS
#undef t_CLASS
#undef CONSTRUCTORCALLBACKINSTALL
#undef CONSTRUCTORCALLBACKUNINSTALL
/* First the case where we are creating the code for the non-inherited fields
 * of the class */
#ifndef BASECLASS 

#ifndef TAGS
#define renamed_t_CLASS _concat_(renamed_t_,CLASS) /* object type */
#define t_CLASS _concat_(t_,CLASS) /* object type */
#endif

#undef DEPTH
#undef CLASS
#undef MANAGER_NAME
#undef MANAGER_TYPE
#undef MANAGER_FIELD

#ifdef TYPEDEFS

#define DIABLO_CLASS_BEGIN typedef struct _t_CLASS
#define MEMBER(x,y,z)
#define MEMBERU(x,y,z)
#define IMEMBER(x,y,z)
#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#define EXTENDS(x)
#define FUNCTION0(x,y)
#define FUNCTION1(x,y,z)
#define FUNCTION2(x,y,z,u)
#define FUNCTION3(x,y,z,u,v)
#define FUNCTION4(x,y,z,u,v,w)
#define FUNCTION5(x,y,z,u,v,w,a)
#define FUNCTION6(x,y,z,u,v,w,a,b)
#define FUNCTION7(x,y,z,u,v,w,a,b,c)

#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)
#define DIABLO_CLASS_END t_CLASS,renamed_t_CLASS;
#endif

#ifdef TYPES
#define DIABLO_CLASS_BEGIN struct _t_CLASS {
#define EXTENDS(x) x _hidden_parent;
#define MEMBER(x,y,z) x _concat_(_hidden_,y);
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) MEMBER(x,y,z)
#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#define FUNCTION0(x,y)
#define FUNCTION1(x,y,z)
#define FUNCTION2(x,y,z,u)
#define FUNCTION3(x,y,z,u,v)
#define FUNCTION4(x,y,z,u,v,w)
#define FUNCTION5(x,y,z,u,v,w,a)
#define FUNCTION6(x,y,z,u,v,w,a,b)
#define FUNCTION7(x,y,z,u,v,w,a,b,c)

#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)
#define DIABLO_CLASS_END };
#endif

#ifdef DEFINES

#define DIABLO_CLASS_BEGIN
#define EXTENDS(x)

#ifdef GEN_CODE_STATIC
#define MEMBER(x,y,z) static inline x _concat3_(_concat_(CLASS,_field_select_prefix),_,z) (const t_CLASS * c) { return c-> _concat_(_hidden_,y); }
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) MEMBER(x,y,z)
#else
#ifdef GEN_CODE
#define MEMBER(x,y,z) x _concat3_(_concat_(CLASS,_field_select_prefix),_,z) (const t_CLASS * c) { return c-> _concat_(_hidden_,y); }
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) static inline MEMBER(x,y,z)
#else /* GEN_PROTO */
#define MEMBER(x,y,z) x _concat3_(_concat_(CLASS,_field_select_prefix),_,z) (const t_CLASS * c);
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) static inline x _concat3_(_concat_(CLASS,_field_select_prefix),_,z) (const t_CLASS * c) { return c-> _concat_(_hidden_,y); }
#endif
#endif

#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#define FUNCTION0(x,y)
#define FUNCTION1(x,y,z)
#define FUNCTION2(x,y,z,u)
#define FUNCTION3(x,y,z,u,v)
#define FUNCTION4(x,y,z,u,v,w)
#define FUNCTION5(x,y,z,u,v,w,a)
#define FUNCTION6(x,y,z,u,v,w,a,b)
#define FUNCTION7(x,y,z,u,v,w,a,b,c)

#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)
#define DIABLO_CLASS_END
#endif

#ifdef DEFINES2
#define DIABLO_CLASS_BEGIN
#define EXTENDS(x)

#ifdef GEN_CODE_STATIC
#define MEMBER(x,y,z) static inline void _concat3_(_concat_(CLASS,_field_select_prefix), _SET_,z) (t_CLASS * c, x val ) { c-> _concat_(_hidden_,y)=val; }
#define MEMBERU(x,y,z) static inline void _concat3_(_concat_(CLASS,_field_select_prefix), _SET_,z) (t_CLASS * c, x val ) { _concat3_(_concat_(CLASS,_function_prefix),Update,z) (c,val); c-> _concat_(_hidden_,y)=val; }
#define IMEMBER(x,y,z) MEMBER(x,y,z)
#else
#ifdef GEN_CODE
#define MEMBER(x,y,z) void _concat3_(_concat_(CLASS,_field_select_prefix), _SET_,z) (t_CLASS * c, x val ) { c-> _concat_(_hidden_,y)=val; }
#define MEMBERU(x,y,z) void _concat3_(_concat_(CLASS,_field_select_prefix), _SET_,z) (t_CLASS * c, x val ) { _concat3_(_concat_(CLASS,_function_prefix),Update,z) (c,val); c-> _concat_(_hidden_,y)=val; }
#define IMEMBER(x,y,z) static inline MEMBER(x,y,z)
#else /* GEN_PROTO */
#define MEMBER(x,y,z) void _concat3_(_concat_(CLASS,_field_select_prefix), _SET_,z) (t_CLASS * c, x val );
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) static inline void _concat3_(_concat_(CLASS,_field_select_prefix), _SET_,z) (t_CLASS * c, x val ) { c-> _concat_(_hidden_,y)=val; }
#endif
#endif

#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#define FUNCTION0(x,y)
#define FUNCTION1(x,y,z)
#define FUNCTION2(x,y,z,u)
#define FUNCTION3(x,y,z,u,v)
#define FUNCTION4(x,y,z,u,v,w)
#define FUNCTION5(x,y,z,u,v,w,a)
#define FUNCTION6(x,y,z,u,v,w,a,b)
#define FUNCTION7(x,y,z,u,v,w,a,b,c)

#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)
#define DIABLO_CLASS_END
#endif

#ifdef FUNCTIONS
#define DIABLO_CLASS_BEGIN
#define EXTENDS(x)
#define MEMBER(x,y,z)
#define MEMBERU(x,y,z)
#define IMEMBER(x,y,z)
#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#define FUNCTION0(x,y) x _concat_(_concat_(CLASS,_function_prefix),y) ();
#define FUNCTION1(x,y,z) x _concat_(_concat_(CLASS,_function_prefix),y) (z);
#define FUNCTION2(x,y,z,u) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u);
#define FUNCTION3(x,y,z,u,v) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v);
#define FUNCTION4(x,y,z,u,v,w) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w);
#define FUNCTION5(x,y,z,u,v,w,a) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a);
#define FUNCTION6(x,y,z,u,v,w,a,b) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a,b);
#define FUNCTION7(x,y,z,u,v,w,a,b,c) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a,b,c);

#define PFUNCTION0(x,y) x _concat_(_concat_(CLASS,_function_prefix),y) ();
#define PFUNCTION1(x,y,z) x _concat_(_concat_(CLASS,_function_prefix),y) (z);
#define PFUNCTION2(x,y,z,u) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u);
#define PFUNCTION3(x,y,z,u,v) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v);
#define PFUNCTION4(x,y,z,u,v,w) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w);
#define PFUNCTION5(x,y,z,u,v,w,a) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a);
#define PFUNCTION6(x,y,z,u,v,w,a,b) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a,b);
#define PFUNCTION7(x,y,z,u,v,w,a,b,c) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a,b,c);
#define DIABLO_CLASS_END
#endif

#ifdef CONSTRUCTORS
#ifdef MANAGER_FIELD
#define DIABLO_CLASS_BEGIN

#else
#define DIABLO_CLASS_BEGIN
#endif

#ifdef GEN_CODE_STATIC
#define CALLBACKPREFIX static
#else
#define CALLBACKPREFIX
#endif


#ifdef GEN_PROTO
#define CONSTRUCTORCALLBACKINSTALL \
CALLBACKPREFIX void _concat_(_concat_(CLASS,_function_prefix),CallbackInstall)(MANAGER_TYPE tp, t_manager_callback_type type, t_int32 prior, t_manager_callback_function fun, void * data);
#else
#define CONSTRUCTORCALLBACKINSTALL \
CALLBACKPREFIX void _concat_(_concat_(CLASS,_function_prefix),CallbackInstall)(MANAGER_TYPE tp, t_manager_callback_type type, t_int32 prior, t_manager_callback_function fun, void * data)\
{\
  t_manager_callback * search=NULL;\
  t_manager_callback * cb=(t_manager_callback*) Malloc(sizeof(t_manager_callback));\
  cb->cb_user_data=data;\
  cb->cb_func=fun;\
  cb->type=type;\
  cb->prior=prior;\
  switch(type)\
  {\
    case CB_NEW:\
                {\
                  search=tp -> _concat_(_hidden_,MANAGER_FIELD) .new_;\
                  if ((!search)||(search->prior>prior))\
                  {\
                    cb->next=search;\
                    tp -> _concat_(_hidden_,MANAGER_FIELD) .new_=cb;\
                    return;\
                  }\
                }\
    break;\
    case CB_FREE:\
                 {\
                   search=tp -> _concat_(_hidden_,MANAGER_FIELD) .free;\
                   if ((!search)||(search->prior>prior))\
                   {\
                     cb->next=search;\
                     tp -> _concat_(_hidden_,MANAGER_FIELD) .free=cb;\
                     return;\
                   }\
                 }\
    break;\
    case CB_DUP:\
                {\
                  search=tp -> _concat_(_hidden_,MANAGER_FIELD) .dup;\
                  if ((!search)||(search->prior>prior))\
                  {\
                    cb->next=search;\
                    tp -> _concat_(_hidden_,MANAGER_FIELD) .dup=cb;\
                    return;\
                  }\
                }\
    break;\
  }\
  while((search->next) && (search->next->prior<prior)) { search=search->next; }\
  cb->next=search->next;\
  search->next=cb;\
}
#endif

#ifdef GEN_PROTO
#define CONSTRUCTORCALLBACKUNINSTALL \
CALLBACKPREFIX void _concat_(_concat_(CLASS,_function_prefix),CallbackUninstall)(MANAGER_TYPE tp, t_manager_callback_type type, t_int32 prior, t_manager_callback_function fun, void * data);
#else
#define CONSTRUCTORCALLBACKUNINSTALL \
CALLBACKPREFIX void _concat_(_concat_(CLASS,_function_prefix),CallbackUninstall)(MANAGER_TYPE tp, t_manager_callback_type type, t_int32 prior, t_manager_callback_function fun, void * data)\
{\
  t_manager_callback * prev=NULL;\
  t_manager_callback * search=NULL;\
  switch(type)\
  {\
    case CB_NEW:\
                {\
                  search=tp -> _concat_(_hidden_,MANAGER_FIELD) .new_;\
                }\
    break;\
    case CB_FREE:\
                 {\
                   search=tp -> _concat_(_hidden_,MANAGER_FIELD) .free;\
                 }\
    break;\
    case CB_DUP:\
                {\
                  search=tp -> _concat_(_hidden_,MANAGER_FIELD) .dup;\
                }\
    break;\
  }\
  while(search)\
  {\
    if ((search->prior==prior)&&(search->cb_func==fun)&&(search->cb_user_data==data))\
    {\
      if (prev) {\
        prev->next=search->next;\
      }\
      else\
      {\
        switch(type)\
        {\
          case CB_NEW:\
                      {\
                        tp -> _concat_(_hidden_,MANAGER_FIELD) .new_=search->next;\
                      }\
          break;\
          case CB_FREE:\
                       {\
                         tp -> _concat_(_hidden_,MANAGER_FIELD) .free=search->next;\
                       }\
          break;\
          case CB_DUP:\
                      {\
                        tp -> _concat_(_hidden_,MANAGER_FIELD) .dup=search->next;\
                      }\
          break;\
        }\
      }\
      Free(search);\
      return;\
    }\
    prev=search;\
    search=search->next;\
  }\
}
#endif

#define EXTENDS(x)
#define MEMBER(x,y,z)
#define MEMBERU(x,y,z)
#define IMEMBER(x,y,z)
#define CONSTRUCTOR(b) \
static t_CLASS * _concat_(_concat_(CLASS,_function_prefix),New) (MANAGER_TYPE MANAGER_NAME)\
{\
  t_CLASS * ret;\
  t_manager_callback * new_cb= MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) . new_;\
  while(new_cb && new_cb->prior<0) { new_cb->cb_func(NULL,new_cb->cb_user_data); new_cb=new_cb->next; }\
  ret=(t_CLASS*) Calloc(1,sizeof(t_CLASS));\
  ret-> _concat_(_hidden_,MANAGER_NAME) = MANAGER_NAME ;\
  { b } while(0);\
  while(new_cb) { new_cb->cb_func((void *) ret,new_cb->cb_user_data); new_cb=new_cb->next; }\
  return ret;\
}\
static t_CLASS * _concat_(_concat_(CLASS,_function_prefix),Init) (t_CLASS * ret, MANAGER_TYPE MANAGER_NAME)\
{\
  t_manager_callback * new_cb= MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) . new_;\
  while(new_cb && new_cb->prior<0) { new_cb->cb_func(NULL,new_cb->cb_user_data); new_cb=new_cb->next; }\
  ret-> _concat_(_hidden_,MANAGER_NAME) = MANAGER_NAME ;\
  { b } while(0);\
  while(new_cb) { new_cb->cb_func((void *) ret,new_cb->cb_user_data); new_cb=new_cb->next; }\
  return ret;\
}\
CONSTRUCTORCALLBACKINSTALL \
CONSTRUCTORCALLBACKUNINSTALL

#define CONSTRUCTOR1(e,r,b) \
static t_CLASS * _concat_(_concat_(CLASS,_function_prefix),New) (MANAGER_TYPE MANAGER_NAME,e r)\
{\
  t_CLASS * ret;\
  t_manager_callback * new_cb= MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) . new_;\
  while(new_cb && new_cb->prior<0) { new_cb->cb_func(NULL,new_cb->cb_user_data); new_cb=new_cb->next; }\
  ret=(t_CLASS*) Calloc(1,sizeof(t_CLASS));\
  ret-> _concat_(_hidden_,MANAGER_NAME) = MANAGER_NAME ;\
  { b } while(0);\
  while(new_cb) { new_cb->cb_func((void *) ret,new_cb->cb_user_data); new_cb=new_cb->next; }\
  return ret;\
}\
static t_CLASS * _concat_(_concat_(CLASS,_function_prefix),Init) (t_CLASS * ret, MANAGER_TYPE MANAGER_NAME,e r)\
{\
  t_manager_callback * new_cb= MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) . new_;\
  while(new_cb && new_cb->prior<0) { new_cb->cb_func(NULL,new_cb->cb_user_data); new_cb=new_cb->next; }\
  ret-> _concat_(_hidden_,MANAGER_NAME) = MANAGER_NAME ;\
  { b } while(0);\
  while(new_cb) { new_cb->cb_func((void *) ret,new_cb->cb_user_data); new_cb=new_cb->next; }\
  return ret;\
}\
CONSTRUCTORCALLBACKINSTALL \
CONSTRUCTORCALLBACKUNINSTALL

#define CONSTRUCTOR2(e,r,f,s,b) \
static t_CLASS * _concat_(_concat_(CLASS,_function_prefix),New) (MANAGER_TYPE MANAGER_NAME,e r,f s)\
{\
  t_CLASS * ret;\
  t_manager_callback * new_cb= MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) . new_;\
  while(new_cb && new_cb->prior<0) { new_cb->cb_func(NULL,new_cb->cb_user_data); new_cb=new_cb->next; }\
  ret=(t_CLASS*) Calloc(1,sizeof(t_CLASS));\
  ret-> _concat_(_hidden_,MANAGER_NAME) = MANAGER_NAME ;\
  { b } while(0);\
  while(new_cb) { new_cb->cb_func((void *) ret,new_cb->cb_user_data); new_cb=new_cb->next; }\
  return ret;\
}\
static t_CLASS * _concat_(_concat_(CLASS,_function_prefix),Init) (t_CLASS * ret, MANAGER_TYPE MANAGER_NAME,e r,f s)\
{\
  t_manager_callback * new_cb= MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) . new_;\
  while(new_cb && new_cb->prior<0) { new_cb->cb_func(NULL,new_cb->cb_user_data); new_cb=new_cb->next; }\
  ret-> _concat_(_hidden_,MANAGER_NAME) = MANAGER_NAME ;\
  { b } while(0);\
  while(new_cb) { new_cb->cb_func((void *) ret,new_cb->cb_user_data); new_cb=new_cb->next; }\
  return ret;\
}\
CONSTRUCTORCALLBACKINSTALL \
CONSTRUCTORCALLBACKUNINSTALL

#define CONSTRUCTOR3(e,r,f,s,g,t,b) \
static t_CLASS * _concat_(_concat_(CLASS,_function_prefix),New) (MANAGER_TYPE MANAGER_NAME,e r,f s,g t)\
{\
  t_CLASS * ret;\
  t_manager_callback * new_cb= MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) . new_;\
  while(new_cb && new_cb->prior<0) { new_cb->cb_func(NULL,new_cb->cb_user_data); new_cb=new_cb->next; }\
  ret=(t_CLASS*) Calloc(1,sizeof(t_CLASS));\
  ret-> _concat_(_hidden_,MANAGER_NAME) = MANAGER_NAME ;\
  { b } while(0);\
  while(new_cb) { new_cb->cb_func((void *) ret,new_cb->cb_user_data); new_cb=new_cb->next; }\
  return ret;\
}\
static t_CLASS * _concat_(_concat_(CLASS,_function_prefix),Init) (t_CLASS * ret, MANAGER_TYPE MANAGER_NAME,e r,f s,g t)\
{\
  t_manager_callback * new_cb= MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) . new_;\
  while(new_cb && new_cb->prior<0) { new_cb->cb_func(NULL,new_cb->cb_user_data); new_cb=new_cb->next; }\
  ret-> _concat_(_hidden_,MANAGER_NAME) = MANAGER_NAME ;\
  { b } while(0);\
  while(new_cb) { new_cb->cb_func((void *) ret,new_cb->cb_user_data); new_cb=new_cb->next; }\
  return ret;\
}\
CONSTRUCTORCALLBACKINSTALL \
CONSTRUCTORCALLBACKUNINSTALL

#define DESTRUCTOR(b) static void _concat_(_concat_(CLASS,_function_prefix),Free) (t_CLASS const * to_free) { MANAGER_TYPE MANAGER_NAME = to_free-> _concat_(_hidden_,MANAGER_NAME); t_manager_callback * free_cb=MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) .free; while(free_cb && free_cb->prior<0) { free_cb->cb_func((void *) to_free,free_cb->cb_user_data); free_cb=free_cb->next; } { b } while(0); Free(to_free); while(free_cb) { free_cb->cb_func(NULL,free_cb->cb_user_data); free_cb=free_cb->next; } } static void _concat_(_concat_(CLASS,_function_prefix),BeforeFree) (t_CLASS * to_free) { MANAGER_TYPE MANAGER_NAME = to_free-> _concat_(_hidden_,MANAGER_NAME); t_manager_callback * free_cb=MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) .free; while(free_cb && free_cb->prior<0) { free_cb->cb_func((void *) to_free,free_cb->cb_user_data); free_cb=free_cb->next; } }

extern void *global_hack_dup_orig;

#define DUPLICATOR(b) \
static t_CLASS * _concat_(_concat_(CLASS,_function_prefix),Dup) (t_CLASS * to_dup) { \
  t_CLASS * ret; \
  MANAGER_TYPE MANAGER_NAME = to_dup-> _concat_(_hidden_,MANAGER_NAME); \
  t_manager_callback * dup_cb=MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) .dup; \
  while(dup_cb && dup_cb->prior<0) { \
    dup_cb->cb_func((void *) to_dup,dup_cb->cb_user_data); \
    dup_cb=dup_cb->next; \
  } \
  ret=(t_CLASS*) Malloc(sizeof(t_CLASS)); \
  memcpy(ret,to_dup,sizeof(t_CLASS)); \
  { b } while(0); \
  global_hack_dup_orig=to_dup; \
  while(dup_cb) { \
    dup_cb->cb_func((void *) ret,dup_cb->cb_user_data); \
    dup_cb=dup_cb->next; \
  } \
  return ret; \
} \
static void _concat_(_concat_(CLASS,_function_prefix),BeforeDup) (t_CLASS * to_dup) { \
  MANAGER_TYPE MANAGER_NAME = to_dup-> _concat_(_hidden_,MANAGER_NAME); \
  t_manager_callback * dup_cb=MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) .dup; \
  while(dup_cb && dup_cb->prior<0) { \
    dup_cb->cb_func((void *) to_dup,dup_cb->cb_user_data); \
    dup_cb=dup_cb->next; \
  } \
} \
static void _concat_(_concat_(CLASS,_function_prefix),AfterDup) (t_CLASS * to_dup, t_CLASS * ret) { \
  MANAGER_TYPE MANAGER_NAME = to_dup-> _concat_(_hidden_,MANAGER_NAME); \
  t_manager_callback * dup_cb=MANAGER_NAME -> _concat_(_hidden_,MANAGER_FIELD) .dup; \
  while(dup_cb && dup_cb->prior<0) { \
    dup_cb=dup_cb->next; \
  } \
  while(dup_cb) { \
    dup_cb->cb_func((void *) ret,dup_cb->cb_user_data); \
    dup_cb=dup_cb->next; \
  }\
}
  
#define FUNCTION0(x,y)
#define FUNCTION1(x,y,z)
#define FUNCTION2(x,y,z,u)
#define FUNCTION3(x,y,z,u,v)
#define FUNCTION4(x,y,z,u,v,w)
#define FUNCTION5(x,y,z,u,v,w,a)
#define FUNCTION6(x,y,z,u,v,w,a,b)
#define FUNCTION7(x,y,z,u,v,w,a,b,c)

#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)
#define DIABLO_CLASS_END

#endif

#ifdef DOC_DERIVED
#define DIABLO_CLASS_BEGIN class t_CLASS
#define EXTENDS(x) : public x { public:
#define MEMBER(x,y,z) x y;
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) MEMBER(x,y,z)
#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#define FUNCTION0(x,y) x _concat_(_concat_(CLASS,_function_prefix),y) ();
#define FUNCTION1(x,y,z) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1);
#define FUNCTION2(x,y,z,u) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2);
#define FUNCTION3(x,y,z,u,v) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3);
#define FUNCTION4(x,y,z,u,v,w) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4);
#define FUNCTION5(x,y,z,u,v,w,a) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4,a p5);
#define FUNCTION6(x,y,z,u,v,w,a,b) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4,a p5,b p6);
#define FUNCTION7(x,y,z,u,v,w,a,b,c) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4,a p5,b p6,c p7);

#define PFUNCTION0(x,y) x _concat_(_concat_(CLASS,_function_prefix),y) ();
#define PFUNCTION1(x,y,z) x _concat_(_concat_(CLASS,_function_prefix),y) (z);
#define PFUNCTION2(x,y,z,u) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u);
#define PFUNCTION3(x,y,z,u,v) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v);
#define PFUNCTION4(x,y,z,u,v,w) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w);
#define PFUNCTION5(x,y,z,u,v,w,a) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a);
#define PFUNCTION6(x,y,z,u,v,w,a,b) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a,b);
#define PFUNCTION7(x,y,z,u,v,w,a,b,c) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a,b,c);
#define DIABLO_CLASS_END 
#endif

#ifdef DOC_BASE
#define DIABLO_CLASS_BEGIN class t_CLASS { public:
#define EXTENDS(x)
#define MEMBER(x,y,z) x y;
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) MEMBER(x,y,z)
#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#define FUNCTION0(x,y) x _concat_(_concat_(CLASS,_function_prefix),y) ();
#define FUNCTION1(x,y,z) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1);
#define FUNCTION2(x,y,z,u) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2);
#define FUNCTION3(x,y,z,u,v) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3);
#define FUNCTION4(x,y,z,u,v,w) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4);
#define FUNCTION5(x,y,z,u,v,w,a) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4,a p5);
#define FUNCTION6(x,y,z,u,v,w,a,b) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4,a p5,b p6);
#define FUNCTION7(x,y,z,u,v,w,a,b,c) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4,a p5,b p6,c p7);

#define PFUNCTION0(x,y) x _concat_(_concat_(CLASS,_function_prefix),y) ();
#define PFUNCTION1(x,y,z) x _concat_(_concat_(CLASS,_function_prefix),y) (z);
#define PFUNCTION2(x,y,z,u) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u);
#define PFUNCTION3(x,y,z,u,v) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v);
#define PFUNCTION4(x,y,z,u,v,w) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w);
#define PFUNCTION5(x,y,z,u,v,w,a) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a);
#define PFUNCTION6(x,y,z,u,v,w,a,b) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a,b);
#define PFUNCTION7(x,y,z,u,v,w,a,b,c) x _concat_(_concat_(CLASS,_function_prefix),y) (z,u,v,w,a,b,c);
#define DIABLO_CLASS_END };
#endif

#ifdef TAGS
#define DIABLO_CLASS_BEGIN CLASS PATTYPE
#define EXTENDS(x)
#define MEMBER(x,y,z) _concat_(CLASS,_field_select_prefix):z PATMEMBER(x,y,z)
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) MEMBER(x,y,z)
#define CONSTRUCTOR(b) _concat_(CLASS,_function_prefix):New+Init PATCONSTRUCTOR
#define CONSTRUCTOR1(e,r,b) _concat_(CLASS,_function_prefix):New+Init PATCONSTRUCTOR
#define CONSTRUCTOR2(e,r,f,s,b) _concat_(CLASS,_function_prefix):New+Init PATCONSTRUCTOR
#define CONSTRUCTOR3(e,r,f,s,g,t,b) _concat_(CLASS,_function_prefix):New+Init PATCONSTRUCTOR
#define DESTRUCTOR(b) _concat_(CLASS,_function_prefix):Free+BeforeFree PATDESTRUCTOR
#define DUPLICATOR(b) _concat_(CLASS,_function_prefix):Dup+BeforeDup+AfterDup PATDUPLICATOR
#define FUNCTION0(x,y) _concat_(CLASS,_function_prefix):y PATFUNCTION 
#define FUNCTION1(x,y,z) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define FUNCTION2(x,y,z,u) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define FUNCTION3(x,y,z,u,v) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define FUNCTION4(x,y,z,u,v,w) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define FUNCTION5(x,y,z,u,v,w,a) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define FUNCTION6(x,y,z,u,v,w,a,b) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define FUNCTION7(x,y,z,u,v,w,a,b,c) _concat_(CLASS,_function_prefix):y PATFUNCTION

#define PFUNCTION0(x,y) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define PFUNCTION1(x,y,z) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define PFUNCTION2(x,y,z,u) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define PFUNCTION3(x,y,z,u,v) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define PFUNCTION4(x,y,z,u,v,w) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define PFUNCTION5(x,y,z,u,v,w,a) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define PFUNCTION6(x,y,z,u,v,w,a,b) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define PFUNCTION7(x,y,z,u,v,w,a,b,c) _concat_(CLASS,_function_prefix):y PATFUNCTION
#define DIABLO_CLASS_END
#endif

/* From here on we are creating the code for the inherited fields
 * of the class */
#else


#ifndef DEPTH
#define DEPTH 1
#undef GET_PARENT
#define GET_PARENT(x)   GetBase(x)
#else
#if DEPTH == 1
#undef DEPTH
#define DEPTH 2
#undef GET_PARENT
#define GET_PARENT(x)   GetBase(GetBase(x))
#elif DEPTH == 2
#undef DEPTH
#define DEPTH 3
#undef GET_PARENT
#define GET_PARENT(x)   GetBase(GetBase(GetBase(x)))
#elif DEPTH == 3
#undef DEPTH
#define DEPTH 4
#undef GET_PARENT
#define GET_PARENT(x)   GetBase(GetBase(GetBase(GetBase(x))))
#elif DEPTH == 4
#undef DEPTH
#define DEPTH 5
#undef GET_PARENT
#define GET_PARENT(x)   GetBase(GetBase(GetBase(GetBase(GetBase(x)))))
#elif DEPTH == 5
#undef DEPTH
#define DEPTH 6
#undef GET_PARENT
#define GET_PARENT(x)   GetBase(GetBase(GetBase(GetBase(GetBase(x)))))
#elif DEPTH == 6
#error Current maximum inheritence depth is 6. You can extend it here
#endif
#endif

#ifndef TAGS
#define renamed_t_CLASS _concat_(t_,CLASS) /* object type */
#define t_CLASS _concat_(t_,BASECLASS) /* object type */
#endif

#ifdef DEFINES
#define DIABLO_CLASS_BEGIN
#define DIABLO_CLASS_END
#define EXTENDS(x)
#define FUNCTION0(x,y)
#define FUNCTION1(x,y,z)
#define FUNCTION2(x,y,z,u)
#define FUNCTION3(x,y,z,u,v)
#define FUNCTION4(x,y,z,u,v,w)
#define FUNCTION5(x,y,z,u,v,w,a)
#define FUNCTION6(x,y,z,u,v,w,a,b)
#define FUNCTION7(x,y,z,u,v,w,a,b,c)
#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)

#ifdef GEN_CODE_STATIC
#define MEMBER(x,y,z) static inline renamed_##x _concat3_(_concat_(CLASS,_field_select_prefix),_,z) (const renamed_t_CLASS * c) { return (renamed_##x) (GET_PARENT(c)-> _concat_(_hidden_,y)); }
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) MEMBER(x,y,z);
#else
#ifdef GEN_CODE 
#define MEMBER(x,y,z) renamed_##x _concat3_(_concat_(CLASS,_field_select_prefix),_,z) (const renamed_t_CLASS * c) { return (renamed_##x) (GET_PARENT(c)-> _concat_(_hidden_,y)); }
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) static inline MEMBER(x,y,z)
#else /* GEN_PROTO */
#define MEMBER(x,y,z) renamed_##x _concat3_(_concat_(CLASS,_field_select_prefix),_,z) (const renamed_t_CLASS * c);
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) static inline renamed_##x _concat3_(_concat_(CLASS,_field_select_prefix),_,z) (const renamed_t_CLASS * c) { return (renamed_##x) (GET_PARENT(c)-> _concat_(_hidden_,y)); }
#endif
#endif

#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#else
#ifdef DEFINES2
#define DIABLO_CLASS_BEGIN
#define DIABLO_CLASS_END
#define EXTENDS(x)
#define FUNCTION0(x,y)
#define FUNCTION1(x,y,z)
#define FUNCTION2(x,y,z,u)
#define FUNCTION3(x,y,z,u,v)
#define FUNCTION4(x,y,z,u,v,w)
#define FUNCTION5(x,y,z,u,v,w,a)
#define FUNCTION6(x,y,z,u,v,w,a,b)
#define FUNCTION7(x,y,z,u,v,w,a,b,c)

#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)

#ifdef GEN_CODE_STATIC
#define MEMBER(x,y,z) static inline void _concat3_(_concat_(CLASS,_field_select_prefix),_SET_,z) (renamed_t_CLASS * c, renamed_##x val) { ((GET_PARENT(c))-> _concat_(_hidden_,y))=(x) val; }
#define MEMBERU(x,y,z) static inline void _concat3_(_concat_(CLASS,_field_select_prefix),_SET_,z) (renamed_t_CLASS * c, renamed_##x val) { _concat3_(_concat_(CLASS,_field_select_prefix),Update,z)(c,val); ((GET_PARENT(c))-> _concat_(_hidden_,y))=(x) val; }
#define IMEMBER(x,y,z) MEMBER(x,y,z)
#else
#ifdef GEN_CODE 
#define MEMBER(x,y,z) void _concat3_(_concat_(CLASS,_field_select_prefix),_SET_,z) (renamed_t_CLASS * c, renamed_##x val) { ((GET_PARENT(c))-> _concat_(_hidden_,y))=(x) val; }
#define MEMBERU(x,y,z) void _concat3_(_concat_(CLASS,_field_select_prefix),_SET_,z) (renamed_t_CLASS * c, renamed_##x val) { _concat3_(_concat_(CLASS,_field_select_prefix),Update,z)(c,val); ((GET_PARENT(c))-> _concat_(_hidden_,y))=(x) val; }
#define IMEMBER(x,y,z) inline MEMBER(x,y,z)
#else /* GEN_PROTO */
#define MEMBER(x,y,z) void _concat3_(_concat_(CLASS,_field_select_prefix),_SET_,z) (renamed_t_CLASS * c, renamed_##x val);
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) static inline void _concat3_(_concat_(CLASS,_field_select_prefix),_SET_,z) (renamed_t_CLASS * c, renamed_##x val) { ((GET_PARENT(c))-> _concat_(_hidden_,y))=(x) val; }
#endif
#endif

#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#else
#ifdef FUNCTIONS
#define DIABLO_CLASS_BEGIN
#define DIABLO_CLASS_END
#define EXTENDS(x)

#ifdef GEN_CODE_STATIC
#define FUNCTION0(x,y) static inline renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) () { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)(); }
#else
#ifdef GEN_CODE 
#define FUNCTION0(x,y) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) () { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)(); }
#else /* GEN_PROTO */
#define FUNCTION0(x,y) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) ();
#endif
#endif

#ifdef GEN_CODE_STATIC
#define FUNCTION1(x,y,z) static inline renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a); }
#else
#ifdef GEN_CODE 
#define FUNCTION1(x,y,z) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a); }
#else /* GEN_PROTO */
#define FUNCTION1(x,y,z) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a); 
#endif
#endif

#ifdef GEN_CODE_STATIC
#define FUNCTION2(x,y,z,u) static inline renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b); }
#else
#ifdef GEN_CODE 
#define FUNCTION2(x,y,z,u) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b); }
#else /* GEN_PROTO */
#define FUNCTION2(x,y,z,u) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b); 
#endif
#endif
								       
#ifdef GEN_CODE_STATIC
#define FUNCTION3(x,y,z,u,v) static inline renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b, (v) c); }
#else
#ifdef GEN_CODE 
#define FUNCTION3(x,y,z,u,v) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b, (v) c); }
#else /* GEN_PROTO */
#define FUNCTION3(x,y,z,u,v) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c);
#endif
#endif
								       
#ifdef GEN_CODE_STATIC
#define FUNCTION4(x,y,z,u,v,w) static inline renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b, (v) c, (w) d); }
#else
#ifdef GEN_CODE 
#define FUNCTION4(x,y,z,u,v,w) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b, (v) c, (w) d); }
#else /* GEN_PROTO */
#define FUNCTION4(x,y,z,u,v,w) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d);
#endif
#endif

#ifdef GEN_CODE_STATIC
#define FUNCTION5(x,y,z,u,v,w,r) static inline renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d, renamed_##r e) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b, (v) c, (w) d, (r) e); }
#else
#ifdef GEN_CODE 
#define FUNCTION5(x,y,z,u,v,w,r) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d, renamed_##r e) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b, (v) c, (w) d, (r) e); }
#else /* GEN_PROTO */
#define FUNCTION5(x,y,z,u,v,w,r) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d, renamed_##r e);
#endif
#endif

#ifdef GEN_CODE_STATIC
#define FUNCTION6(x,y,z,u,v,w,r,s) static inline renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d, renamed_##r e, renamed_##s f) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b, (v) c, (w) d, (r) e, (s) f); }
#else
#ifdef GEN_CODE 
#define FUNCTION6(x,y,z,u,v,w,r,s) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d, renamed_##r e, renamed_##s f) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b, (v) c, (w) d, (r) e, (s) f); }
#else /* GEN_PROTO */
#define FUNCTION6(x,y,z,u,v,w,r,s) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d, renamed_##r e, renamed_##s f);
#endif
#endif

#ifdef GEN_CODE_STATIC
#define FUNCTION7(x,y,z,u,v,w,r,s,t) static inline renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d, renamed_##r e, renamed_##s f, renamed_##t g) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b, (v) c, (w) d, (r) e, (s) f, (t) g); }
#else
#ifdef GEN_CODE 
#define FUNCTION7(x,y,z,u,v,w,r,s,t) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d, renamed_##r e, renamed_##s f, renamed_##t g) { return (renamed_##x) _concat_(_concat_(BASECLASS,_function_prefix),y)((z) a, (u) b, (v) c, (w) d, (r) e, (s) f, (t) g); }
#else /* GEN_PROTO */
#define FUNCTION7(x,y,z,u,v,w,r,s,t) renamed_##x _concat_(_concat_(CLASS,_function_prefix),y) (renamed_##z a, renamed_##u b, renamed_##v c, renamed_##w d, renamed_##r e, renamed_##s f, renamed_##t g);
#endif
#endif

#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)
#define MEMBER(x,y,z)
#define MEMBERU(x,y,z)
#define IMEMBER(x,y,z)
#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#else
#ifdef CONSTRUCTORS
#define DIABLO_CLASS_BEGIN
#define EXTENDS(x)
#define MEMBER(x,y,z)
#define MEMBERU(x,y,z)
#define IMEMBER(x,y,z)
#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b) static renamed_t_CLASS * _concat_(_concat_(CLASS,_function_prefix),Dup) (renamed_t_CLASS * to_dup) { return (renamed_t_CLASS *) _concat_(_concat_(BASECLASS,_function_prefix),Dup) ((t_CLASS *) to_dup); }
#define FUNCTION0(x,y)
#define FUNCTION1(x,y,z)
#define FUNCTION2(x,y,z,u)
#define FUNCTION3(x,y,z,u,v)
#define FUNCTION4(x,y,z,u,v,w)
#define FUNCTION5(x,y,z,u,v,w,a)
#define FUNCTION6(x,y,z,u,v,w,a,b)
#define FUNCTION7(x,y,z,u,v,w,a,b,c)

#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)
#define DIABLO_CLASS_END
#else
#ifdef TAGS
#define DIABLO_CLASS_BEGIN
#define EXTENDS(x)
#define MEMBER(x,y,z) _concat_(CLASS,_field_select_prefix):z -> BASECLASS
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) MEMBER(x,y,z)
#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#define FUNCTION0(x,y) _concat_(CLASS,_function_prefix):y +> BASECLASS
#define FUNCTION1(x,y,z) _concat_(CLASS,_function_prefix):y +> BASECLASS
#define FUNCTION2(x,y,z,u) _concat_(CLASS,_function_prefix):y +> BASECLASS
#define FUNCTION3(x,y,z,u,v) _concat_(CLASS,_function_prefix):y +> BASECLASS
#define FUNCTION4(x,y,z,u,v,w) _concat_(CLASS,_function_prefix):y +> BASECLASS
#define FUNCTION5(x,y,z,u,v,w,a) _concat_(CLASS,_function_prefix):y +> BASECLASS
#define FUNCTION6(x,y,z,u,v,w,a,b) _concat_(CLASS,_function_prefix):y +> BASECLASS
#define FUNCTION7(x,y,z,u,v,w,a,b,c) _concat_(CLASS,_function_prefix):y +> BASECLASS

#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)
#define DIABLO_CLASS_END
#else

                                                           /* We need to skip the documentation of the derived classes, as doxygen tends
                                                            * to merge this documentation with the documentation of the next class */
#ifdef DOC_DERIVED
#define MEMBER(x,y,z) x y;
#define MEMBERU(x,y,z) MEMBER(x,y,z)
#define IMEMBER(x,y,z) MEMBER(x,y,z)
#define FUNCTION0(x,y) x _concat_(_concat_(CLASS,_function_prefix),y) ();
#define FUNCTION1(x,y,z) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1);
#define FUNCTION2(x,y,z,u) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2);
#define FUNCTION3(x,y,z,u,v) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3);
#define FUNCTION4(x,y,z,u,v,w) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4);
#define FUNCTION5(x,y,z,u,v,w,a) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4,a p5);
#define FUNCTION6(x,y,z,u,v,w,a,b) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4,a p5,b p6);
#define FUNCTION7(x,y,z,u,v,w,a,b,c) x _concat_(_concat_(CLASS,_function_prefix),y) (z p1,u p2,v p3,w p4,a p5,b p6,c p7);

#define EXTENDS(x)	ignore();
#define PFUNCTION0(x,y) ignore();
#define PFUNCTION1(x,y,z) ignore(); 
#define PFUNCTION2(x,y,z,u) ignore();
#define PFUNCTION3(x,y,z,u,v) ignore();
#define PFUNCTION4(x,y,z,u,v,w) ignore();
#define PFUNCTION5(x,y,z,u,v,w,a) ignore();
#define PFUNCTION6(x,y,z,u,v,w,a,b) ignore();
#define PFUNCTION7(x,y,z,u,v,w,a,b,c) ignore();
#define DIABLO_CLASS_BEGIN ignore();

#define DIABLO_CLASS_END };
#else
#ifdef DOC_BASE
#define DIABLO_CLASS_END void ignore() {}
#else
#define DIABLO_CLASS_END
#define MEMBER(x,y,z)
#define MEMBERU(x,y,z)
#define IMEMBER(x,y,z)
#define FUNCTION0(x,y)
#define FUNCTION1(x,y,z)
#define FUNCTION2(x,y,z,u)
#define FUNCTION3(x,y,z,u,v)
#define FUNCTION4(x,y,z,u,v,w)
#define FUNCTION5(x,y,z,u,v,w,a)
#define FUNCTION6(x,y,z,u,v,w,a,b)
#define FUNCTION7(x,y,z,u,v,w,a,b,c)

#define PFUNCTION0(x,y)
#define PFUNCTION1(x,y,z)
#define PFUNCTION2(x,y,z,u)
#define PFUNCTION3(x,y,z,u,v)
#define PFUNCTION4(x,y,z,u,v,w)
#define PFUNCTION5(x,y,z,u,v,w,a)
#define PFUNCTION6(x,y,z,u,v,w,a,b)
#define PFUNCTION7(x,y,z,u,v,w,a,b,c)
#define EXTENDS(x)
#define DIABLO_CLASS_BEGIN
#endif
#endif

#define CONSTRUCTOR(b)
#define CONSTRUCTOR1(e,r,b)
#define CONSTRUCTOR2(e,r,f,s,b)
#define CONSTRUCTOR3(e,r,f,s,g,t,b)
#define DESTRUCTOR(b)
#define DUPLICATOR(b)
#endif
#endif
#endif
#endif
#endif
#endif
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
