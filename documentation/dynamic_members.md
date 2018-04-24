# Dynamic members
The dynamic member mechanism enables the extension of a managed class
with an additional member. Many analyses require members which may not
be part of the core class. To solve this problem, the dynamic member
mechanism can be used. A number of steps are needed to enable the usage
of the member:

1.  An instance of the type `t_dynamic_member_info` needs to be defined
    to keep track of the new member
2.  An `Init`, `Fini` and `Dup` function need to be written to define
    the behaviour when an instance of the class is created, killed or
    duplicated
3.  A `DYNAMIC_MEMBER` declaration of the new member

This will trigger the creation of an initialization and a finalization
function. The usage of the member is transparent (identical to core
members) once this initialization function has been called until a call
to the finalization function is executed. Note that these auto-generated
functions are different from the user-defined functions in step 2. The
former are used to add and remove the member to the class, while the
latter are called with each creation/destruction of an instance of the
class. The member can then be further used as follows:

4.  A call to the auto-generated initialization function will create the
    member for all instances of the class
5.  The member can now be used as if it were part of the core class
6.  The member can be removed using the auto-generated finalization
    function

We illustrate this through a small example: suppose we want to extend
the basic block datastructure with an additional member of the type
boolean. This boolean will be used in a reachability analysis and is
therefore called reachable. We proceed as follows:

~~~~
/*step 1*/
t_dynamic_member_info bbl_reachable_array = null_info;

/*step 2*/
void 
BblReachableInit(t_bbl * bbl, t_bool * reachable)
{
  *reachable = FALSE;
}

void 
BblReachableFini(t_bbl * bbl, t_bool * reachable)
{
  return;
}

void 
BblReachableDup(t_bbl * bbl, t_bool * reachable)
{
  return;
}

/*step 3*/
DYNAMIC_MEMBER(
    bbl,                  /*The datastructure to which the member will be added, without the 't_' prefix*/
    t_cfg *,              /*the manager of the datastructure*/
    bbl_reachable_array,  /*an instance of t_dynamic_member_info to store the members*/
    t_bool,               /*the type of the member*/
    reachable,            /*the name of the member in lower case*/
    REACHABLE,            /*the name of the member in upper case*/
    Reachable,            /*the name of the member in regular case*/
    CFG_FOREACH_BBL,      /*an iterator which iterates over all instances of the managed class contained in the manager class*/
    BblReachableInit,     /*The function to be called when an instance of the class is created*/
    BblReachableFini,     /*The function to be called when an instance of the class is killed*/
    BblReachableDup       /*The function to be called when an instance of the class is duplicated*/
);
~~~~


This will create the functions `BblInitReachable` and `BblFiniReachable`

~~~~
/*step 4*/
BblInitReachable(cfg);

/*step 5*/
/*examples of the usage of the field*/
if(BBL_REACHABLE(bbl))
{
  ...
}

BBL_SET_REACHABLE(bbl, TRUE);

/*step 6*/
BblFiniReachable(cfg);
~~~~

Note that the function `BblInitReachable` and `BblFiniReachable` should
be called from within the same file if you want to avoid memory leaks.
At the moment, a hack is needed to duplicate the new member!

~~~~
void 
BblReachableDup(t_bbl * bbl, t_bool * reachable)
{
  t_bbl * orig_bbl=global_hack_dup_orig;
  *reachable=BBL_REACHABLE(orig_bbl);
  return;
}
~~~~
