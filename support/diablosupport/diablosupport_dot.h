/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* Dot support
 *
 * Interface:
 *
 * Types:
 *
 * TODO
 *
 * Functions:
 *
 * t_dot_graph * DotGraphRead(FILE *) : TODO
 * void DotGraphWrite(t_dot_graph *,FILE ,FILE **) : TODO
 *
 * Defines:
 */

#include <diablosupport.h>
#include <stdio.h>
/* Dot Typedefs {{{ */
#ifndef DIABLOSUPPORT_DOT_TYPEDEFS
#define DIABLOSUPPORT_DOT_TYPEDEFS
typedef struct _t_dot_properties t_dot_properties;
typedef struct _t_dot_node t_dot_node;
typedef struct _t_dot_edge t_dot_edge;
typedef enum
{ DOT_NODE, DOT_EDGE } t_dot_graph_elem_types;
typedef struct _t_dot_graph_elem t_dot_graph_elem;
typedef struct _t_dot_graph t_dot_graph;
typedef struct _t_dot_pos t_dot_pos;
#endif /* }}} Dot Typedefs */
/* Dot Types {{{ */
#ifndef DIABLOSUPPORT_DOT_TYPES
#define DIABLOSUPPORT_DOT_TYPES

/*! \brief A dot element position descriptor
 *
 * struct used to describe a position of a dot graph element (a list for
 * edges, with one element for nodes */
struct _t_dot_pos
{
  double x;
  double y;
  struct _t_dot_pos *next;
};

/*! \brief A dot element properties descriptor
 *
 * struct used to describe the properties of a dot graph element */
struct _t_dot_properties
{
  char dir;
  char *label;
  int bbx;
  int bby;
  int bbw;
  int bbh;
  char *color;
  char *fillcolor;
  char *style;
  char *shape;
  t_dot_pos *pos;
  double width;
  double height;
};

/*! \brief A dot node
 *
 * struct used to describe a dot graph node */
struct _t_dot_node
{
  char *name;
  char *label;
  int x, y;
  float w, h;
  char *color;
  char *style;
  char *fillcolor;
  char *shape;
};

/*! \brief A dot edge
 *
 * struct used to describe a dot graph edge */
struct _t_dot_edge
{
  char dir;
  char *from;
  char *to;
  t_dot_pos *pos;
};

/*! \brief A dot graph element
 *
 * struct used to describe a dot graph element (a node or an edge) */
struct _t_dot_graph_elem
{
  t_dot_graph_elem_types type;
  union
  {
    t_dot_node node;
    t_dot_edge edge;
  } select;
  struct _t_dot_graph_elem *next;
};

/*! \brief A dot graph
 *
 * struct used to describe a dot graph */
struct _t_dot_graph
{
  struct _t_dot_graph_elem *first;
};

#endif /* }}} Dot Types */
#ifdef DIABLOSUPPORT_FUNCTIONS
/* Dot Functions {{{ */
#ifndef DIABLOSUPPORT_FILE_FUNCTIONS
#define DIABLOSUPPORT_FILE_FUNCTIONS
t_dot_graph *DotGraphRead (FILE *);
void DotGraphWrite (const t_dot_graph *, FILE *);
#endif /* }}} Dot Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
