/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport.h>

extern FILE *DotParserin;
extern t_dot_graph_elem *parsed_graph;
extern int DotParserparse(void);

t_dot_graph *
DotGraphRead (FILE * in)
{
  t_dot_graph *ret = Malloc (sizeof (t_dot_graph));

  DotParserin = in;
  parsed_graph = NULL;
  DotParserparse ();
  ret->first = parsed_graph;
  parsed_graph = NULL;
  return ret;
}

void
DotGraphWrite (const t_dot_graph * graph, FILE * out)
{
  t_dot_graph_elem *iter;
  fprintf (out, "digraph out {\n");

  for (iter = graph->first; iter != NULL; iter = iter->next)
  {
    if (iter->type == DOT_NODE)
    {
      fprintf (out, "\"%s\" [ ", iter->select.node.name);
      if (iter->select.node.label)
        fprintf (out, "label=\"%s\" ", iter->select.node.label);
      if (iter->select.node.color != NULL)
      {
        fprintf (out, "color=\"%s\" ", iter->select.node.color);
      }
      if (iter->select.node.fillcolor != NULL)
      {
        fprintf (out, "fillcolor=\"%s\" ", iter->select.node.fillcolor);
      }
      if (iter->select.node.style != NULL)
      {
        fprintf (out, "style=\"%s\" ", iter->select.node.style);
      }
      if (iter->select.node.shape != NULL)
      {
        fprintf (out, "shape=\"%s\" ", iter->select.node.shape);
      }

      if (iter->select.node.w != -1)
      {
        fprintf (out, "width=\"%f\" ", iter->select.node.w);
      }

      if (iter->select.node.h != -1)
      {
        fprintf (out, "height=\"%f\" ", iter->select.node.h);
      }
      fprintf (out, "]\n");
    }
    else if (iter->type == DOT_EDGE)
    {
      fprintf (out, "\"%s\" -> \"%s\"\n", iter->select.edge.from,
               iter->select.edge.to);
    }
  }
  fprintf (out, "}\n");
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
