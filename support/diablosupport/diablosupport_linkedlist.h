/* Linked List Support
 *
 * Interface: TODO
 */

#include <diablosupport.h>

/* Linked List Typedefs {{{ */
#ifndef DIABLOSUPPORT_LINKEDLIST_TYPEDEFS
#define DIABLOSUPPORT_LINKEDLIST_TYPEDEFS
typedef enum
{ SLIST, DLIST, XLIST } t_list_type;
typedef struct _t_list t_list;
typedef struct _t_slist_node t_slist_node;
typedef struct _t_slist t_slist;
typedef struct _t_dlist_node t_dlist_node;
typedef struct _t_dlist t_dlist;
typedef struct _t_xlist_node t_xlist_node;
typedef struct _t_xlist t_xlist;
#endif /* }}} Linked List Typedefs */
/* Linked List Defines {{{ */
#ifndef DIABLOSUPPORT_LINKEDLIST_DEFINES
#define DIABLOSUPPORT_LINKEDLIST_DEFINES
#define XorSwap(x,y) ((x) ^= (y) ^= (x) ^= (y))
#define T_SLIST(x) ((t_slist *) x)
#define T_DLIST(x) ((t_dlist *) x)
#define T_LIST(x) ((t_list *) x)
#define T_XLIST(x) ((t_xlist *) x)
#define T_SLIST_NODE(x) ((t_slist_node *) x)
#define T_DLIST_NODE(x) ((t_dlist_node *) x)
#define T_XLIST_NODE(x) ((t_xlist_node *) x)
#define ListGetElem(list,in) ((void *) (((char *) (in)) + list->node_offset))
#define SListGetElem(list,in) T_SLIST_NODE(ListGetElem(list,in))
#define DListGetElem(list,in) T_DLIST_NODE(ListGetElem(list,in))
#define XListGetElem(list,in) T_XLIST_NODE(ListGetElem(list,in))
#define SlistForNext(list,node,tmp) node=((void *) SListGetElem(T_SLIST(list),node)->next)
#define DlistForNext(list,node,tmp) node=((void *) DListGetElem(T_DLIST(list),node)->next)
#define DlistForPrev(list,node,tmp) node=((void *) DListGetElem(T_DLIST(list),node)->prev)
#define XlistForNext(list,node,tmp) ((((t_pointer_int) tmp)^=(t_pointer_int) (XListGetElem(list,node)->pointer))+XorSwap(((t_pointer_int) tmp),((t_pointer_int ) node)))
#define XlistForPrev(list,node,tmp) ((((t_pointer_int) tmp)^=(t_pointer_int) (XListGetElem(list,node)->pointer))+XorSwap(((t_pointer_int) tmp),((t_pointer_int ) node)))
#define SLIST_FOREACH_NODE(list, node,tmp) for (tmp=NULL, node=T_SLIST(list)->first; node!=NULL; SlistForNext(list,node,tmp))
#define DLIST_FOREACH_NODE(list, node,tmp) for (tmp=NULL, node=T_DLIST(list)->first; node!=NULL; DlistForNext(list,node,tmp))
#define DLIST_FOREACH_NODE_R(list, node,tmp) for (tmp=NULL, node=T_DLIST(list)->last; node!=NULL; DlistForPrev(list,node,tmp))
#define XLIST_FOREACH_NODE(list, node,tmp) for (tmp=NULL, node=T_XLIST(list)->first; node!=NULL; XlistForNext(list,node,tmp))
#define XLIST_FOREACH_NODE_R(list, node,tmp) for (tmp=NULL, node=T_XLIST(list)->last; node!=NULL; XlistForPrev(list,node,tmp))
#define LIST_FOREACH_NODE(list, node,tmp) for (tmp=NULL,node=T_XLIST(list)->first;node!=NULL;list->type==XLIST?(void *) XlistForNext(list,node,tmp):((list->type==DLIST?(DlistForNext(list,node,tmp)):(SlistForNext(list,node,tmp)))))
#define LIST_FOREACH_NODE_R(list, node,tmp) for (tmp=NULL,node=T_XLIST(list)->last; node!=NULL;list->type==XLIST?(void *) XlistForPrev(list,node,tmp):((list->type==DLIST?(DlistForPrev(list,node,tmp)):(SlistForNext(list,node,tmp)))))
#define ListAppend(l,e) do { if (T_LIST(l)->type==DLIST) DlistAppend(T_DLIST(l),T_DLIST_NODE(e)); else if (T_LIST(l)->type==XLIST) XlistAppend(T_XLIST(l),T_XLIST_NODE(e)); else FATAL(("Called append on the wrong list type!")); } while(0)
#endif /* }}} Linked List Defines */
/* Linked List Types {{{ */
#ifndef DIABLOSUPPORT_LINKEDLIST_TYPES
#define DIABLOSUPPORT_LINKEDLIST_TYPES
/*! A node in a single linked list */
struct _t_slist_node
{
  struct _t_slist_node *next;
};

/*! A node in a double linked list */
struct _t_dlist_node
{
  struct _t_dlist_node *next;
  struct _t_dlist_node *prev;
};

/*! A node in a xor linked list */
struct _t_xlist_node
{
  t_pointer_int pointer;
};

/*! Generic reference to a list (single, double or xor linked) */
struct _t_list
{
  t_list_type type;
  int node_offset;
  void *first;
};

/*! Reference to a single linked list */
struct _t_slist
{
  t_list_type type;
  int node_offset;
  void *first;
};

/*! Reference to a double linked list */
struct _t_dlist
{
  t_list_type type;
  int node_offset;
  void *first;
  void *last;
};

/*! Reference to a xorlinked list */
struct _t_xlist
{
  t_list_type type;
  int node_offset;
  void *first;
  void *last;
};
#endif /* }}} Linked List types */
#ifdef DIABLOSUPPORT_FUNCTIONS
#ifndef DIABLOSUPPORT_LINKEDLIST_FUNCTIONS /* Linked List Functions {{{ */
#define DIABLOSUPPORT_LINKEDLIST_FUNCTIONS
t_slist *SlistNew ();
t_dlist *DlistNew ();
t_xlist *XlistNew ();
void SlistAppend (t_slist *, void *);
void DlistAppend (t_dlist *, void *);
void XlistAppend (t_xlist *, void *);
#endif /* Linked List Functions }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
