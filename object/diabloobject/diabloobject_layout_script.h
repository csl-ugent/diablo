/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/** \file
 *
 * Layouting an objectfile is the process of assigning address to all the
 * different object entities (sections, symbols, ... ). Diabloobject uses
 * scripts to guide this process. This file holds the typedefs, types,
 * function, and defines needed to interpret and parse these scripts. */

/* Layout Script Typedefs {{{ */
#ifndef DIABLOOBJECT_LAYOUT_SCRIPT_TYPEDEFS
#define DIABLOOBJECT_LAYOUT_SCRIPT_TYPEDEFS
typedef struct _t_layout_exp t_layout_exp;
typedef struct _t_layout_assign t_layout_assign;
typedef struct _t_layout_secspec t_layout_secspec;
typedef struct _t_layout_rule t_layout_rule;
typedef struct _t_layout_script t_layout_script;
#endif /* Layout Script Typedefs }}} */
#include <diabloobject.h>
#ifdef DIABLOOBJECT_TYPES
/* Layout Script Types {{{ */
#ifndef DIABLOOBJECT_LAYOUT_SCRIPT_TYPES
#define DIABLOOBJECT_LAYOUT_SCRIPT_TYPES
/*! \todo Document */
struct _t_layout_exp
{
  int operator_;
  struct _t_layout_exp *arg1;
  struct _t_layout_exp *arg2;
  t_string identifier;
  long long constant;
};

/*! \todo Document */
struct _t_layout_assign
{
  t_string lhs;
  t_layout_exp *rhs;
  t_bool provide;
};

/*! \todo Document */
struct _t_layout_secspec
{
  t_string name;
  t_layout_exp *address;
  t_layout_assign *internal_rule;
  t_layout_exp *filler_exp;
  t_bool wildcard;
};

/*! Holds one rule from the layout description */
struct _t_layout_rule
{
  enum
  { ASSIGN, SECTION_SPEC, SEG_START, SEG_END, PUT_SECTIONS, OVERLAY_START, OVERLAY_END } kind;
  union
  {
    t_layout_secspec *secspec;
    t_layout_assign *assign;
    t_string segment_ident;
    char sectype;
  } u;
  struct _t_layout_rule *next;
};

/*! A parsed layout script */
struct _t_layout_script
{
  /*! The first layout rule in the layout script */
  struct _t_layout_rule *first;
};
#endif /* }}} */
#endif
#ifdef DIABLOOBJECT_FUNCTIONS
/* Layout Script Functions {{{ */
#ifndef DIABLOOBJECT_LAYOUT_SCRIPT_FUNCTIONS
#define DIABLOOBJECT_LAYOUT_SCRIPT_FUNCTIONS
t_layout_script *LayoutScriptParse (t_const_string script);
long long LayoutScriptCalcExp (const t_layout_exp * exp, long long currpos, t_object * obj);
t_layout_rule *LayoutScriptGetRuleForSection(const t_layout_script *script, const t_section *sec);
void LayoutScriptFree (const t_layout_script * script);
#endif /* }}} Layout Script Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
