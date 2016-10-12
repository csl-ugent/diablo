/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport.h>

t_hash_table * conf_hash;
extern FILE* ConfParserin;

int ConfParserparse(void);

typedef struct 
{
  t_hash_table_node node;
  t_string  string;
} t_conf_value;

void 
ConfValueFree(const void * tf, void *unused_data)
{
  const t_conf_value* cv=tf;
  Free(cv->string);
  Free(HASH_TABLE_NODE_KEY(&cv->node));
  Free(cv);
}

t_conf_value * 
ConfValueNew(t_string x, t_string y)
{
  t_conf_value * ret=Malloc(sizeof(t_conf_value));
  HASH_TABLE_NODE_SET_KEY(&ret->node, x);
  ret->string=y;
  return ret;
}

void
ConfValueSet(t_string x, t_string y)
{
  HashTableInsert(conf_hash,ConfValueNew(x,y));
}

t_string 
ConfValueGet(t_hash_table * in,t_string x)
{
  t_conf_value * y;
  if(!in) return NULL;
  y=((t_conf_value *) HashTableLookup(in,x));
  if (y) return y->string;
  return NULL;
}

t_hash_table * 
ConfFileRead(t_const_string conffile)
{
  FILE * f = fopen(conffile,"r");
  if (!f) return NULL;
  ConfParserin = f;
  conf_hash=HashTableNew(7,0,(t_hash_func)StringHash, (t_hash_cmp)StringCmp,ConfValueFree);
  ConfParserparse();
  return conf_hash;	
}

void 
ConfValuePrint(const void * string1, void * string2)
{
  const t_conf_value * value = (const t_conf_value*)string1;
  VERBOSE(0,("(%s,%s)",HASH_TABLE_NODE_KEY(&value->node),value->string));
}
